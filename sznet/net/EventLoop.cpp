#include "EventLoop.h"
#include "Poller.h"
#include "Channel.h"
#include "../process/Process.h"
#include "../log/Logging.h"
#include "../time/TimerQueue.h"

#include <algorithm>
#include <thread>

#if defined(SZ_OS_LINUX)
#	include <signal.h>
#endif

namespace sznet
{

namespace net
{

// 记录当前线程的事件循环对象
thread_local EventLoop* t_loopInThisThread = 0;
// 默认reactor tick时间
const int kPollTimeMs = 10000;

// linux下忽略pipe信号
class IgnoreSigPipe
{
public:
	IgnoreSigPipe()
	{
#ifdef SZ_OS_LINUX 
		::signal(SIGPIPE, SIG_IGN);
#endif
	}
};
IgnoreSigPipe initObj;

EventLoop::EventLoop(): 
	m_looping(false),
	m_quit(false),
	m_eventHandling(false),
	m_callingPendingFunctors(false),
	m_iteration(0),
	m_threadId(CurrentThread::tid()),
	m_poller(Poller::newDefaultPoller(this)),
	m_timerQueue(new TimerQueue(this)),
	m_wakeupFd(sockets::sz_create_eventfd()),
#if defined(SZ_OS_WINDOWS)
	m_wakeupChannel(new Channel(this, m_wakeupFd.event_read)),
#else
	m_wakeupChannel(new Channel(this, m_wakeupFd)),
#endif
	m_currentActiveChannel(nullptr)
{
	LOG_DEBUG << "EventLoop created " << this << " in thread " << m_threadId;
	if (t_loopInThisThread)
	{
		LOG_FATAL << "Another EventLoop " << t_loopInThisThread
			<< " exists in this thread " << m_threadId;
	}
	else
	{
		t_loopInThisThread = this;
	}
	m_wakeupChannel->setReadCallback(std::bind(&EventLoop::handleRead, this));
	m_wakeupChannel->enableReading();
}

EventLoop::~EventLoop()
{
	LOG_DEBUG << "EventLoop " << this << " of thread " << m_threadId
		<< " destructs in thread " << CurrentThread::tid();
	assertInLoopThread();
	for (const Functor& functor : m_pendingFunctors)
	{
		functor();
	}
	// 停止fd所有事件
	m_wakeupChannel->disableAll();
	// poll中也移除
	m_wakeupChannel->remove();
	sockets::sz_close_eventfd(m_wakeupFd);
	t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
	assert(!m_looping);
	assertInLoopThread();
	m_looping = true;
	m_quit = false;
	LOG_TRACE << "EventLoop " << this << " start looping";

	while (!m_quit)
	{
		m_activeChannels.clear();
		m_pollTime = Timestamp::now();
		// IO
		m_poller->poll(&m_activeChannels, m_timerQueue->earliestExpiredTime(m_pollTime, kPollTimeMs));
		++m_iteration;
		if (Logger::logLevel() <= Logger::TRACE)
		{
			printActiveChannels();
		}
		m_eventHandling = true;
		// 处理到期timer
		m_timerQueue->expiredProcess(m_pollTime);
		// 处理就绪事件
		for (Channel* channel : m_activeChannels)
		{
			m_currentActiveChannel = channel;
			m_currentActiveChannel->handleEvent(m_pollTime);
		}
		m_currentActiveChannel = nullptr;
		m_eventHandling = false;
		// 执行m_pendingFunctors中的任务回调
		// 这种设计使得IO线程也能执行一些计算任务，避免了IO线程在不忙时长期阻塞在IO multiplexing调用中
		// 这些任务显然必须是要求在IO线程中执行
		doPendingFunctors();
		doPerTickFunctors();
	}
	
	LOG_TRACE << "EventLoop " << this << " stop looping";
	m_looping = false;
}

void EventLoop::quit()
{
	m_quit = true;
	// 不是EventLoop所在线程，需要通知一下EventLoop所在线程
	if (!isInLoopThread())
	{
		wakeup();
	}
}

void EventLoop::runInLoop(Functor cb)
{
	// 如果是当前IO线程调用runInLoop，则同步调用cb
	if (isInLoopThread())
	{
		cb();
	}
	// 如果是其它线程调用runInLoop，则异步地将cb添加到队列
	else
	{
		queueInLoop(std::move(cb));
	}
}

int EventLoop::runPerTick(PerTickFuncWrap* ptfw)
{
	assert(ptfw);
	if (isInLoopThread())
	{
		ptfw->idx = static_cast<int>(m_perTickFunctos.size());
		m_perTickFunctos.emplace_back(ptfw);
		wakeup();
		return 0;
	}
	LOG_ERROR << "Call runPerTick must in loop thread";
	return -1;
}

int EventLoop::delRunPerTick(PerTickFuncWrap* ptfw)
{
	assert(ptfw);
	if (isInLoopThread())
	{
		auto idx = ptfw->idx;
		assert(0 <= idx && idx < static_cast<int>(m_perTickFunctos.size()));
		if (implicit_cast<size_t>(idx) == m_perTickFunctos.size() - 1)
		{
			m_perTickFunctos.pop_back();
		}
		else
		{
			auto lastElem = m_perTickFunctos.back();
			std::iter_swap(m_perTickFunctos.begin() + idx, m_perTickFunctos.end() - 1);
			lastElem->idx = idx;
			m_perTickFunctos.pop_back();
		}
		return 0;
	}
	LOG_ERROR << "Call delRunPerTick must in loop thread";
	return -1;
}

void EventLoop::queueInLoop(Functor cb)
{
	// 把任务加入到队列可能同时被多个线程调用，需要加锁
	{
		MutexLockGuard lock(m_mutex);
		m_pendingFunctors.push_back(std::move(cb));
	}

	// 1.调用queueInLoop的线程不是当前IO线程则需要唤醒当前IO线程，才能及时执行doPendingFunctors();
	// 2.调用queueInLoop的线程是当前IO线程（比如在doPendingFunctors()中执行functors[i]()时又调用了queueInLoop()）
	// 并且此时正在调用pending functor，需要唤醒当前IO线程，因为在此时doPendingFunctors() 过程中又添加了任务，
	// 故循环回去poll的时候需要被唤醒返回，进而继续执行doPendingFunctors()
	// 3.只有在当前IO线程的事件回调中调用queueInLoop才不需要唤醒
	// 即在handleEvent()中调用queueInLoop不需要唤醒，因为接下来马上就会执行doPendingFunctors();
	if (!isInLoopThread() || m_callingPendingFunctors)
	{
		wakeup();
	}
}

size_t EventLoop::queueSize() const
{
	MutexLockGuard lock(m_mutex);
	return m_pendingFunctors.size();
}

TimerId EventLoop::runAt(Timestamp time, TimerCallback cb)
{
	return m_timerQueue->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delay, TimerCallback cb)
{
	Timestamp time(addTime(Timestamp::now(), delay));
	return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(double interval, TimerCallback cb)
{
	Timestamp time(addTime(Timestamp::now(), interval));
	return m_timerQueue->addTimer(std::move(cb), time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
	return m_timerQueue->cancel(timerId);
}

void EventLoop::updateChannel(Channel* channel)
{
	assert(channel->ownerLoop() == this);
	assertInLoopThread();
	m_poller->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
	assert(channel->ownerLoop() == this);
	assertInLoopThread();
	if (m_eventHandling)
	{
		assert(m_currentActiveChannel == channel || std::find(m_activeChannels.begin(), m_activeChannels.end(), channel) == m_activeChannels.end());
	}
	m_poller->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
	assert(channel->ownerLoop() == this);
	assertInLoopThread();
	return m_poller->hasChannel(channel);
}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
	return t_loopInThisThread;
}

void EventLoop::abortNotInLoopThread()
{
	LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
		<< " was created in m_threadId = " << m_threadId
		<< ", current thread id = " << CurrentThread::tid();
}

void EventLoop::wakeup()
{
	uint64_t one = 1;
	int n = 0;
#if defined(SZ_OS_WINDOWS)
	n = ::send(m_wakeupFd.event_write, reinterpret_cast<const char*>(&one), sizeof(one), 0);
#else
	n = ::write(m_wakeupFd, &one, sizeof(one));
#endif
	if (n > 0) 
	{
		if (n != sizeof(one))
		{
			LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
		}
		return;
	}
	else if (n == 0) 
	{
		LOG_ERROR << "EventLoop::wakeup() send return equal zero";
	}
	else 
	{
		if (sz_getlasterr() == sz_err_eintr || sockets::sz_wouldblock())
		{
			//LOG_WARN << "EventLoop::wakeup() send sz_getlasterr() rst " 
			//	<< sz_getlasterr() 
			//	<< " is sz_err_eintr or sockets::sz_wouldblock()";
			return;
		}
		LOG_ERROR << "EventLoop::wakeup() send error " << sz_getlasterr();
	}
}

void EventLoop::handleRead()
{
	uint64_t one = 1;
	int n = 0;
#if defined(SZ_OS_WINDOWS)
	n = ::recv(m_wakeupFd.event_read, reinterpret_cast<char*>(&one), sizeof(one), 0);
#else
	n = ::read(m_wakeupFd, (char*)&one, sizeof(one));
#endif
	if (n > 0)
	{
		if (n != sizeof(one))
		{
			LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
		}
		return;
	}
	else if (n == 0)
	{
		LOG_ERROR << "EventLoop::handleRead() m_wakeupFd has been closed";
	}
	else
	{
		if (sz_getlasterr() == sz_err_eintr || sockets::sz_wouldblock())
		{
			LOG_WARN << "EventLoop::handleRead() recv sz_getlasterr() rst " 
				<< sz_getlasterr() 
				<< " is sz_err_eintr or sockets::sz_wouldblock()";
			return;
		}
		LOG_ERROR << "EventLoop::handleRead() recv error";
	}
}

void EventLoop::doPendingFunctors()
{
	std::vector<Functor> functors;
	m_callingPendingFunctors = true;

	{
		MutexLockGuard lock(m_mutex);
		// 不是简单地在临界区依次调用Functor，而是把回调列表swap()到局部变量functors中，
		// 这样做，一方面减小了临界区的长度(不会阻塞其他线程调用queueInLoop())，
		// 另一方面避免了死锁(因为Functor可能再调用queueInLoop())。
		functors.swap(m_pendingFunctors);
	}

	for (const Functor& functor : functors)
	{
		functor();
	}

	m_callingPendingFunctors = false;
}

void EventLoop::doPerTickFunctors()
{
	for (const auto& wrap : m_perTickFunctos)
	{
		wrap->func();
	}
}

void EventLoop::printActiveChannels() const
{
	for (const Channel* channel : m_activeChannels)
	{
		LOG_TRACE << "{" << channel->reventsToString() << "} ";
	}
}

} // end namespace net

} // end namespace sznet