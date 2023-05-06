#include "ThreadClass.h"
#include "CurrentThread.h"
#include "../log/Logging.h"
#include "../base/Exception.h"
// move
#include <utility>

namespace sznet
{

namespace detail
{

// 主要用于主线程初始化
class ThreadNameInitializer
{
public:
	ThreadNameInitializer()
	{
		// 设置主线程名称
		sznet::CurrentThread::t_threadName = "main";
		// 缓存线程id
		m_mainPid = CurrentThread::tid();
	}
	sz_pid_t m_mainPid = sz_invalid_pid;
};
ThreadNameInitializer init;

// 线程数据类
struct ThreadData
{
	// 线程工作函数类型
	typedef sznet::Thread::ThreadFunc ThreadFunc;
	// 线程工作函数
	ThreadFunc m_func;
	// 线程名
	string m_name;
	// 线程id，给创建线程的thread对象中m_tid赋值
	sz_pid_t* m_tid;
	// 计数器，给创建线程的thread对象中倒计时计数器减一，唤醒
	CountDownLatch* m_latch;

	ThreadData(ThreadFunc func, const string& name, sz_pid_t* tid, CountDownLatch* latch):
		m_func(std::move(func)),
		m_name(name),
		m_tid(tid),
		m_latch(latch)
	{ 
	}

	// 线程运行时候跑的函数
	void runInThread()
	{
		// 获得该线程的tid，没有缓存就重新获取再缓存
		*m_tid = sznet::CurrentThread::tid();
		m_tid = nullptr;
		// -1，唤醒
		m_latch->countDown();
		m_latch = nullptr;

		sznet::CurrentThread::t_threadName = m_name.empty() ? "sznetThread" : m_name.c_str();
		try
		{
			m_func();
			sznet::CurrentThread::t_threadName = "finished";
		}
		catch (const Exception& ex)
		{
			sznet::CurrentThread::t_threadName = "crashed";
			fprintf(stderr, "exception caught in Thread %s\n", m_name.c_str());
			fprintf(stderr, "reason: %s\n", ex.what());
			fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
			abort();
		}
		catch (const std::exception& ex)
		{
			sznet::CurrentThread::t_threadName = "crashed";
			fprintf(stderr, "exception caught in Thread %s\n", m_name.c_str());
			fprintf(stderr, "reason: %s\n", ex.what());
			abort();
		}
		catch (...)
		{
			sznet::CurrentThread::t_threadName = "crashed";
			fprintf(stderr, "unknown exception caught in Thread %s\n", m_name.c_str());
			// re throw
			throw;
		}
	}
};

// 线程函数
// 首先将参数转型为ThreadData*，然后调用data->runInThread()
sz_thread_func_return startThread(void* obj)
{
	ThreadData* data = static_cast<ThreadData*>(obj);
	data->runInThread();
	delete data;
	data = nullptr;

	return 0;
}

} // namespace detail

bool CurrentThread::isMainThread()
{
#if defined(SZ_OS_WINDOWS)
	return detail::init.m_mainPid == GetCurrentThreadId();
#else
	return tid() == sz_getpid();
#endif
}

std::atomic<int> Thread::m_numCreated(0);

// 一般传递普通函数指针或者std::function都会拷贝生成
// func，所以使用std::move节省资源
Thread::Thread(ThreadFunc func, const string& n):
	m_started(false),
	m_joined(false),
	m_pthreadId(0),
	m_tid(0),
	m_func(std::move(func)),
	m_name(n),
	m_latch(1)
{
	setDefaultName();
}

Thread::~Thread()
{
	if (m_started && !m_joined)
	{
		sz_thread_detach(m_pthreadId);
	}
}

void Thread::setDefaultName()
{
	int num = ++m_numCreated;
	if (m_name.empty())
	{
		char buf[32];
		snprintf(buf, sizeof(buf), "Thread%d", num);
		m_name = buf;
	}
}

void Thread::start()
{
	assert(!m_started);
	m_started = true;
	// FIXME: move(m_func)
	detail::ThreadData* data = new detail::ThreadData(m_func, m_name, &m_tid, &m_latch);
	m_pthreadId = sz_thread_create(&detail::startThread, data);
	if (m_pthreadId < 0)
	{
		m_started = false;
		delete data;
		data = nullptr;
		LOG_SYSFATAL << "Failed in thread create";
	}
	else
	{
		m_latch.wait();
		assert(m_tid > 0);
	}
}

int Thread::join()
{
	assert(m_started);
	assert(!m_joined);
	m_joined = true;
	return sz_waitfor_thread_terminate(m_pthreadId);
}

} // end namespace sznet