#include "EventLoopThread.h"
#include "EventLoop.h"

namespace sznet
{

namespace net
{

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const string& name): 
	m_loop(nullptr),
	m_exiting(false),
	m_thread(std::bind(&EventLoopThread::threadFunc, this), name),
	m_mutex(),
	m_cond(m_mutex),
	m_callback(cb)
{
}

EventLoopThread::~EventLoopThread()
{
	m_exiting = true;
	if (m_loop != nullptr)
	{
		// 通知完成以后就等
		m_loop->quit();
		m_thread.join();
	}
}

EventLoop* EventLoopThread::startLoop()
{
	assert(!m_thread.started());
	m_thread.start();

	EventLoop* loop = nullptr;
	// 线程创建好了，还是要等等IO事件循环对象的创建
	{
		MutexLockGuard lock(m_mutex);
		while (m_loop == nullptr)
		{
			m_cond.wait();
		}
		loop = m_loop;
	}

	return loop;
}

void EventLoopThread::threadFunc()
{
	EventLoop loop;

	// 循环前的回调任务
	if (m_callback)
	{
		m_callback(&loop);
	}

	// 创建好了，给EventLoopThread对象通知一哈
	{
		MutexLockGuard lock(m_mutex);
		m_loop = &loop;
		m_cond.notify();
	}

	loop.loop();
	// 结束
	MutexLockGuard lock(m_mutex);
	m_loop = nullptr;
}

} // end namespace net

} // end namespace sznet

