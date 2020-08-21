#include "ThreadPool.h"
#include "../base/Exception.h"

#include <assert.h>
#include <stdio.h>

namespace sznet
{

ThreadPool::ThreadPool(const string& nameArg) :
	m_mutex(),
	m_notEmpty(m_mutex),
	m_notFull(m_mutex),
	m_name(nameArg),
	m_maxQueueSize(0),
	m_running(false)
{
}

ThreadPool::~ThreadPool()
{
	if (m_running)
	{
		stop();
	}
}

void ThreadPool::start(int numThreads)
{
	assert(m_threads.empty());
	m_running = true;
	// 预留 numThreads 个空间
	m_threads.reserve(numThreads);
	// 创建 numThreads 个数的线程，并启动
	for (int i = 0; i < numThreads; ++i)
	{
		char id[32];
		snprintf(id, sizeof(id), "%d", i + 1);
		// 创建线程并绑定函数和指定线程名
		m_threads.emplace_back(new sznet::Thread(std::bind(&ThreadPool::runInThread, this), m_name + id));
		// 启动
		m_threads[i]->start();
	}
	// 不打算开启线程来处理任务，并且有处理任务前的回调函数
	// 主线程执行回调函数，因为主线程需要自己执行任务
	if (numThreads == 0 && m_threadInitCallback)
	{
		m_threadInitCallback();
	}
}

void ThreadPool::stop()
{
	{
		MutexLockGuard lock(m_mutex);
		m_running = false;
		// 大家都停止
		m_notEmpty.notifyAll();
	}
	// 等待多有子线程都执行完成后，停止
	for (auto& thr : m_threads)
	{
		thr->join();
	}
}

size_t ThreadPool::queueSize() const
{
	MutexLockGuard lock(m_mutex);
	return m_queue.size();
}

void ThreadPool::run(Task task)
{
	// 如果线程池为空，说明线程池未分配线程
	// 主线程执行任务
	if (m_queue.empty())
	{
		task();
	}
	else
	{
		MutexLockGuard lock(m_mutex);
		// 当任务队列满的时候，循环等待
		while (isFull())
		{
			// 一直等待任务队列不满
			// 这个锁在线程take()取任务函数中时，
			// 会通知主线程，这里就唤醒了
			m_notFull.wait();
		}
		assert(!isFull());
		// 当任务队列不满，就把该任务加入线程池的任务队列
		m_queue.push_back(std::move(task));
		// 通知线程池中的任一线程，谁抢到就是谁的
		m_notEmpty.notify();
	}
}

ThreadPool::Task ThreadPool::take()
{
	MutexLockGuard lock(m_mutex);
	// 线程已经启动但是任务队列为空（无任务），则等待，
	// 直到任务队列不为空或running_为false
	while (m_queue.empty() && m_running)
	{
		m_notEmpty.wait();
	}
	Task task;
	if (!m_queue.empty())
	{
		// 取出队列头的任务
		task = m_queue.front();
		// 并从队列删除该任务
		m_queue.pop_front();
		// 任务队列有大小，既然取出来，
		// 告诉主线程可以继续放入了
		if (m_maxQueueSize > 0)
		{
			// 通知
			m_notFull.notify();
		}
	}
	return task;
}

bool ThreadPool::isFull() const
{
	m_mutex.assertLocked();
	return m_maxQueueSize > 0 && m_queue.size() >= m_maxQueueSize;
}

void ThreadPool::runInThread()
{
	try
	{
		// 如果有回调函数，先调用回调函数。
		// 为任务执行做准备
		if (m_threadInitCallback)
		{
			m_threadInitCallback();
		}
		while (m_running)
		{
			// 取出任务
			// 有可能阻塞在这里，因为任务队列为空
			Task task(take());
			if (task)
			{
				// 执行任务
				task();
			}
		}
	}
	catch (const Exception& ex)
	{
		fprintf(stderr, "exception caught in ThreadPool %s\n", m_name.c_str());
		fprintf(stderr, "reason: %s\n", ex.what());
		fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
		abort();
	}
	catch (const std::exception& ex)
	{
		fprintf(stderr, "exception caught in ThreadPool %s\n", m_name.c_str());
		fprintf(stderr, "reason: %s\n", ex.what());
		abort();
	}
	catch (...)
	{
		fprintf(stderr, "unknown exception caught in ThreadPool %s\n", m_name.c_str());
		throw;
	}
}

} // end namespace sznet
