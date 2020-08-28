#ifndef _SZNET_NET_EVENTLOOP_H_
#define _SZNET_NET_EVENTLOOP_H_

#include "../thread/Mutex.h"
#include "../thread/CurrentThread.h"
#include "../time/Timestamp.h"
#include "../base/NonCopyable.h"
#include "../base/Callbacks.h"
#include "../time/TimerId.h"
#include "../net/SocketsOps.h"

#include <atomic>
#include <functional>
#include <vector>

namespace sznet
{

// 定时器队列
class TimerQueue;

namespace net
{

// 事件分发
class Channel;
// 多路复用
class Poller;

// 事件循环
class EventLoop : NonCopyable
{
public:
	// 
	typedef std::function<void()> Functor;

	EventLoop();
	~EventLoop();

	// 事件循环
	void loop();
	// 终止事件循环
	void quit();
	// 
	// Timestamp pollReturnTime() const { return pollReturnTime_; }
	// 
	// int64_t iteration() const { return iteration_; }
	// 
	void runInLoop(Functor cb);
	// 
	void queueInLoop(Functor cb);
	// 
	// size_t queueSize() const;
	// timers
	// 
	TimerId runAt(Timestamp time, TimerCallback cb);
	// 
	TimerId runAfter(double delay, TimerCallback cb);
	// 
	TimerId runEvery(double interval, TimerCallback cb);
	// 取消定时器
	void cancel(TimerId timerId);
	// 唤醒EventLoop所在的线程，用来停止事件循环
	void wakeup();
	// 更新channel的事件
	void updateChannel(Channel* channel);
	// 移除channel
	void removeChannel(Channel* channel);
	// bool hasChannel(Channel* channel);
	// pid_t threadId() const { return threadId_; }
	// 断言运行loop函数的线程必须是拥有EventLoop对象的线程
	void assertInLoopThread()
	{
		// 若运行线程不拥有EventLoop对象则退出，保证one loop per thread
		if (!isInLoopThread())
		{
			abortNotInLoopThread();
		}
	}
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
	// m_wakeupFd可读时回调函数
	void handleRead();
	// 
	void doPendingFunctors();
	// 
	void printActiveChannels() const;
	// 
	typedef std::vector<Channel*> ChannelList;
	// 事件循环是否运行
	bool m_looping;
	// 事件循环前会判断该变量，退出事件循环标记
	std::atomic<bool> m_quit;
	// 是否正在处理事件
	bool m_eventHandling;
	// 是否正在执行用户任务函数回调
	bool m_callingPendingFunctors;
	// loop次数
	int64_t m_iteration;
	// 当前线程的tid
	const sz_pid_t m_threadId;
	// 
	Timestamp pollReturnTime_;
	// 多路复用对象
	std::unique_ptr<Poller> m_poller;
	// 定时器队列用于存放定时器
	std::unique_ptr<TimerQueue> m_timerQueue;
	// 唤醒EventLoop所在的线程
	sockets::sz_event m_wakeupFd;
	// m_wakeupFd对应的事件分发对象
	std::unique_ptr<Channel> m_wakeupChannel;
	// 
	// boost::any context_;
	// 
	ChannelList m_activeChannels;
	// 当前正在处理的就绪事件
	Channel* m_currentActiveChannel;
	// 
	mutable MutexLock mutex_;
	// 
	std::vector<Functor> pendingFunctors_;
};

} // end namespace net

} // end namespace muduo

#endif  // MUDUO_NET_EVENTLOOP_H
