// Comment: 事件循环实现

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
// one loop per thread
// 一个线程最多只能有一个事件循环，这个线程叫做IO线程
class EventLoop : NonCopyable
{
public:
	// 来自其他线程或者本身线程的执行函数类型
	typedef std::function<void()> Functor;

	EventLoop();
	virtual ~EventLoop();

	// 事件循环，该函数不能跨线程调用
	// 只能在创建该对象的线程中调用
	void loop();
	// 终止事件循环
	// 可以跨线程调用
	void quit();
	// 返回IO多路复用的时间
	Timestamp pollReturnTime() const 
	{ 
		return m_pollTime; 
	}
	// 返回IO循环次数
	int64_t iteration() const 
	{ 
		return m_iteration; 
	}
	// 在IO线程中执行某个回调函数，该函数可以跨线程调用
	void runInLoop(Functor cb);
	// 来自其他线程的回调函数放入队列中等待执行
	void queueInLoop(Functor cb);
	// 返回必须在IO线程中执行的函数数组的大小
	size_t queueSize() const;
	// timers相关
	// 在某个绝对时间点执行定时回调
	TimerId runAt(Timestamp time, TimerCallback cb);
	// 相对当前时间delay执行定时回调
	TimerId runAfter(double delay, TimerCallback cb);
	// 每隔interval执行定时回调
	TimerId runEvery(double interval, TimerCallback cb);
	// 取消定时器
	void cancel(TimerId timerId);
	// 唤醒EventLoop所在的线程，用来停止事件循环
	void wakeup();
	// 更新channel的事件
	void updateChannel(Channel* channel);
	// 移除channel
	void removeChannel(Channel* channel);
	// 是否有channel
	bool hasChannel(Channel* channel);
	// 返回创建loop对象的线程ID
	sz_pid_t threadId() const 
	{
		return m_threadId; 
	}
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
	// 返回是否正在执行用户任务函数(来自其他线程的请求)回调
	bool callingPendingFunctors() const 
	{ 
		return m_callingPendingFunctors; 
	}
	// 是否正在处理poll后的事件
	bool eventHandling() const 
	{ 
		return m_eventHandling; 
	}
	// 当前线程的事件循环
	static EventLoop* getEventLoopOfCurrentThread();

protected:
	// 在不拥有EventLoop对象的线程中终止程序
	void abortNotInLoopThread();
	// m_wakeupFd可读时回调函数
	void handleRead();
	// 执行待执行数组中的任务
	// 该函数只会被EventLoop对象所在的线程执行
	void doPendingFunctors();
	// 调试打印就绪事件数组
	void printActiveChannels() const;
	// 就绪事件数组类型
	typedef std::vector<Channel*> ChannelList;
	// 事件循环是否运行
	bool m_looping;
	// 事件循环前会判断该变量，退出事件循环标记
	std::atomic<bool> m_quit;
	// 是否正在处理poll后的事件
	bool m_eventHandling;
	// 是否正在执行用户任务函数(来自其他线程的请求)回调
	bool m_callingPendingFunctors;
	// loop次数
	int64_t m_iteration;
	// 当前线程的tid
	const sz_pid_t m_threadId;
	// IO多路复用的时间
	Timestamp m_pollTime;
	// 多路复用对象
	std::unique_ptr<Poller> m_poller;
	// 定时器队列用于存放定时器
	std::unique_ptr<TimerQueue> m_timerQueue;
	// 唤醒EventLoop所在的线程
	sockets::sz_event m_wakeupFd;
	// m_wakeupFd对应的事件分发对象
	std::unique_ptr<Channel> m_wakeupChannel;
	// 就绪事件集合
	ChannelList m_activeChannels;
	// 当前正在处理的就绪事件
	Channel* m_currentActiveChannel;
	// 互斥量
	mutable MutexLock m_mutex;
	// 需要在IO线程中执行的函数数组
	// 这些任务显然必须是要求在IO线程中执行
	std::vector<Functor> m_pendingFunctors;
};

} // end namespace net

} // end namespace sznet

#endif  // _SZNET_NET_EVENTLOOP_H_
