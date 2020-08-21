#ifndef _SZNET_NET_EVENTLOOP_H_
#define _SZNET_NET_EVENTLOOP_H_

#include "../thread/Mutex.h"
#include "../thread/CurrentThread.h"
#include "../time/Timestamp.h"
#include "../base/NonCopyable.h"
#include "../base/Callbacks.h"
#include "../time/TimerId.h"

#include <atomic>
#include <functional>
#include <vector>

namespace sznet
{

namespace net
{

// 
class Channel;
// 
class Poller;
// 
class TimerQueue;

// 
class EventLoop : NonCopyable
{
public:
	// 
	typedef std::function<void()> Functor;

	EventLoop();
	~EventLoop();

	// 
	// void loop();
	// 终止事件循环
	void quit();
	// 
	// Timestamp pollReturnTime() const { return pollReturnTime_; }
	// 
	// int64_t iteration() const { return iteration_; }
	// 
	// void runInLoop(Functor cb);
	// 
	// void queueInLoop(Functor cb);
	// 
	// size_t queueSize() const;
	// timers
	// 
	TimerId runAt(Timestamp time, TimerCallback cb);
	// 
	TimerId runAfter(double delay, TimerCallback cb);
	// 
	TimerId runEvery(double interval, TimerCallback cb);
	// 
	// void cancel(TimerId timerId);
	// 唤醒EventLoop所在的线程，用来停止事件循环
	void wakeup();
	// void updateChannel(Channel* channel);
	// void removeChannel(Channel* channel);
	// bool hasChannel(Channel* channel);
	// pid_t threadId() const { return threadId_; }
	// 运行loop函数的线程必须是拥有EventLoop的线程
	void assertInLoopThread()
	{
		if (!isInLoopThread())
		{
			abortNotInLoopThread();
		}
	}
	// 
	// 
	// 判断当前线程是否是拥有此EventLoop的线程
	bool isInLoopThread() const 
	{ 
		return m_threadId == CurrentThread::tid();
	}
	// bool callingPendingFunctors() const { return callingPendingFunctors_; }
	// bool eventHandling() const { return eventHandling_; }
	// void setContext(const boost::any& context)
	// {
	//	context_ = context;
	// }
	// const boost::any& getContext() const
	// {
	// 	return context_;
	// }
	// 
	// boost::any* getMutableContext()
	// {
	// 	return &context_;
	// }
	// 
	// static EventLoop* getEventLoopOfCurrentThread();

private:
	// 在不拥有EventLoop线程中终止
	void abortNotInLoopThread();
	// 
	void handleRead();
	// 
	void doPendingFunctors();
	// 
	void printActiveChannels() const;
	// 
	typedef std::vector<Channel*> ChannelList;
	// 事件循环是否运行标记
	bool m_looping;
	// 事件循环前会判断该标记
	std::atomic<bool> m_quit;
	// 
	bool eventHandling_;
	// 
	bool callingPendingFunctors_;
	// 
	int64_t iteration_;
	// 当前线程的tid
	const sz_pid_t m_threadId;
	// 
	Timestamp pollReturnTime_;
	// 
	std::unique_ptr<Poller> poller_;
	// 
	std::unique_ptr<TimerQueue> timerQueue_;
	// 
	// int wakeupFd_;
	// 
	// std::unique_ptr<Channel> wakeupChannel_;
	// 
	// boost::any context_;
	// 
	ChannelList activeChannels_;
	// 
	Channel* currentActiveChannel_;
	// 
	mutable MutexLock mutex_;
	// 
	std::vector<Functor> pendingFunctors_;
};

} // end namespace net

} // end namespace muduo

#endif  // MUDUO_NET_EVENTLOOP_H
