#include "EventLoop.h"
#include "Poller.h"
#include "Channel.h"
#include "../log/Logging.h"
#include "../time/TimerQueue.h"

#include <algorithm>
#include <thread>

namespace sznet
{

namespace net
{

// 记录事件循环对象
thread_local EventLoop* t_loopInThisThread = 0;

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
#elif defined(SZ_OS_LINUX)
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
	// 
	m_wakeupChannel->disableAll();
	// 
	m_wakeupChannel->remove();
	sockets::sz_close_eventfd(m_wakeupFd);
	t_loopInThisThread = NULL;
}

//
void EventLoop::loop()
{
//	assert(!looping_);
//	assertInLoopThread();
//	looping_ = true;
//	quit_ = false;  // FIXME: what if someone calls quit() before loop() ?
//	LOG_TRACE << "EventLoop " << this << " start looping";
//
//	while (!quit_)
//	{
//		activeChannels_.clear();
//		pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
//		++iteration_;
//		if (Logger::logLevel() <= Logger::TRACE)
//		{
//			printActiveChannels();
//		}
//		// TODO sort channel by priority
//		eventHandling_ = true;
//		for (Channel* channel : activeChannels_)
//		{
//			currentActiveChannel_ = channel;
//			currentActiveChannel_->handleEvent(pollReturnTime_);
//		}
//		currentActiveChannel_ = NULL;
//		eventHandling_ = false;
//		doPendingFunctors();
//	}
//
//	LOG_TRACE << "EventLoop " << this << " stop looping";
//	looping_ = false;
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
//	if (isInLoopThread())
//	{
//		cb();
//	}
//	else
//	{
//		queueInLoop(std::move(cb));
//	}
}

void EventLoop::queueInLoop(Functor cb)
{
//	{
//		MutexLockGuard lock(mutex_);
//		pendingFunctors_.push_back(std::move(cb));
//	}
//
//	if (!isInLoopThread() || callingPendingFunctors_)
//	{
//		wakeup();
//	}
}

//size_t EventLoop::queueSize() const
//{
//	MutexLockGuard lock(mutex_);
//	return pendingFunctors_.size();
//}
//

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

//bool EventLoop::hasChannel(Channel* channel)
//{
//	assert(channel->ownerLoop() == this);
//	assertInLoopThread();
//	return poller_->hasChannel(channel);
//}

void EventLoop::abortNotInLoopThread()
{
	LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
		<< " was created in m_threadId = " << m_threadId
		<< ", current thread id = " << CurrentThread::tid();
}

void EventLoop::wakeup()
{
	//uint64_t one = 1;
	//ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
	//if (n != sizeof one)
	//{
	//	LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
	//}
}

void EventLoop::handleRead()
{
	uint64_t one = 1;
	int n = recv(m_wakeupFd.event_read, (char*)&one, sizeof(one), 0);
	if (n != sizeof(one))
	{
		LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
	}
}

void EventLoop::doPendingFunctors()
{
//	std::vector<Functor> functors;
//	callingPendingFunctors_ = true;
//
//	{
//		MutexLockGuard lock(mutex_);
//		functors.swap(pendingFunctors_);
//	}
//
//	for (const Functor& functor : functors)
//	{
//		functor();
//	}
//	callingPendingFunctors_ = false;
}

void EventLoop::printActiveChannels() const
{
//	for (const Channel* channel : activeChannels_)
//	{
//		LOG_TRACE << "{" << channel->reventsToString() << "} ";
//	}
}

} // end namespace net

} // end namespace sznet