// Comment: 线程IO事件循环实现

#ifndef _SZNET_NET_EVENTLOOPTHREAD_H_
#define _SZNET_NET_EVENTLOOPTHREAD_H_

#include "../thread/Condition.h"
#include "../thread/Mutex.h"
#include "../thread/ThreadClass.h"
#include "../base/NonCopyable.h"

namespace sznet
{

namespace net
{

// IO事件循环
class EventLoop;

// 线程IO事件循环
class EventLoopThread : NonCopyable
{
public:
	// 线程IO线程循环前的回调函数类型
	typedef std::function<void(EventLoop*)> ThreadInitCallback;

	EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(), const string& name = string());
	~EventLoopThread();

	// 开启IO事件循环
	EventLoop* startLoop();

private:
	// IO事件循环工作线程
	void threadFunc();

	// 指向IO事件循环对象
	EventLoop* m_loop;
	// 是否退出
	bool m_exiting;
	// 线程对象，用于创建m_loop的工作线程
	Thread m_thread;
	// 配合条件变量保护m_loop
	MutexLock m_mutex;
	Condition m_cond;
	// 线程IO事件循环前的回调函数
	ThreadInitCallback m_callback;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_EVENTLOOPTHREAD_H_

