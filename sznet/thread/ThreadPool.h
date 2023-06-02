// Comment: 线程池实现

#ifndef _SZNET_THREAD_THREADPOOL_H_
#define _SZNET_THREAD_THREADPOOL_H_

#include "../base/NonCopyable.h"
#include "Condition.h"
#include "Mutex.h"
#include "ThreadClass.h"
#include <deque>
#include <vector>

namespace sznet
{

// 简单的固定大小线程池，不能自动伸缩
// 任务队列大小不能自动伸缩
// 线程池大小不能自动伸缩
class ThreadPool : NonCopyable
{
public:
	// 任务
	typedef std::function<void()> Task;

public:
	// 构造函数，默认线程池名字为ThreadPool
	explicit ThreadPool(const string& nameArg = string("ThreadPool"));
	~ThreadPool();

	// 设置任务队列允许的大小
	// Must be called before start().
	void setMaxQueueSize(int maxSize) 
	{ 
		m_maxQueueSize = maxSize; 
	}
	// 设置线程执行任务前的回调函数
	void setThreadInitCallback(const Task& cb)
	{
		m_threadInitCallback = cb;
	}
	// 启动固定的线程数目的线程池 
	void start(int numThreads);
	// 关闭线程池
	void stop();
	// 返回线程池名称
	const string& name() const
	{
		return m_name;
	}
	// 任务队列当前大小
	size_t queueSize() const;
	// 运行一个任务，所以这个线程池执行任务是靠任务队列，
	// 客户需要执行一个任务，必须首先将该任务push进任务队列，等侯空闲线程处理
	void run(Task f);

private:
	// 判断任务队列是否已满
	bool isFull() const;
	// 线程池中每个线程执行的函数
	// 线程运行函数，无任务时都会阻塞在take()，有任务时会争互斥锁
	void runInThread();
	// 线程从队列获取任务
	Task take();

	// 配合条件变量，保护m_queue
	// <<Linux系统编程手册>>也有这个问题的介绍：
	// A condition variable is always used in conjunction with a mutex. 
	// The mutex provides mutual exclusion for accessing the shared variable, 
	// while the condition variable is used to signal changes in the variable’s state.
	mutable MutexLock m_mutex;
	// 非空条件变量
	Condition m_notEmpty;
	// 未满条件变量
	Condition m_notFull;
	// 线程池名称
	string m_name;
	// 线程执行任务前的回调函数
	Task m_threadInitCallback;
	// 线程池容器
	std::vector<std::unique_ptr<Thread>> m_threads;
	// 任务队列
	std::deque<Task> m_queue;
	// 因为deque是通过push_back增加任务数目，所以需要通过 m_maxQueueSize 限制数目
	size_t m_maxQueueSize;
	// 线程池是否处于运行转态
	bool m_running;
};

} // end namespace sznet

#endif // _SZNET_THREAD_THREADPOOL_H_
