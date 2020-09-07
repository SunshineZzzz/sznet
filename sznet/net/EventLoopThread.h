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

// IO事件循环线程
// 任何一个线程，只要创建并运行了EventLoop，就是一个IO线程。
// EventLoopThread类就是一个封装好了的IO线程。
class EventLoopThread : NonCopyable
{
public:
	// IO线程循环前的回调函数
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
	// m_loop循环前的回调函数
	ThreadInitCallback m_callback;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_EVENTLOOPTHREAD_H_

