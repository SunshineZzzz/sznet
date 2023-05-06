// Comment: 互斥量实现

#ifndef _SZNET_THREAD_MUTEX_H_
#define _SZNET_THREAD_MUTEX_H_

#include "Thread.h"
#include "CurrentThread.h"
#include "../base/NonCopyable.h"
#include <assert.h>

namespace sznet
{

// mutex封裝
class MutexLock : NonCopyable
{
public:
	MutexLock(): 
		m_holder(sz_invalid_pid)
	{
		sz_mutex_init(&m_mutex);
	}
	~MutexLock()
	{
		assert(m_holder == sz_invalid_pid);
		sz_mutex_fini(&m_mutex);
	}

	// 判断当前线程是否拥有互斥量
	// must be called when locked, i.e. for assertion
	bool isLockedByThisThread() const
	{
		return m_holder == CurrentThread::tid();
	}
	// 断言是否已经锁上
	void assertLocked() const
	{
		assert(isLockedByThisThread());
	}
	// internal usage
	// 上锁并设置锁的持有者
	void lock()
	{
		sz_mutex_lock(&m_mutex);
		assignHolder();
	}
	// 解锁并解除锁的持有者
	void unlock()
	{
		unassignHolder();
		sz_mutex_unlock(&m_mutex);
	}
	// 获得锁，条件变量有时候需要锁，因此返回其地址
	sz_mutex_t* getPthreadMutex()
	{
		return &m_mutex;
	}

private:
	// 友元Condition，为的是其能操作MutexLock类
	friend class Condition;
	// 用于条件变量上锁后，执行wait操作时，解除线程绑定，防止其他线程上锁时调用失败
	// 配合Condition Variable
	class UnassignGuard : NonCopyable
	{
	public:
		explicit UnassignGuard(MutexLock& owner): 
			m_owner(owner)
		{
			// 在执行等待之前，使用UnassignGuard的构造函数将mutex的holder清空
			// (因为当前线程会休眠，暂时失去对mutex的所有权)。
			// 接着调用pthread_cond_wait等待其他线程的通知。
			// 当其他某个线程调用了notify/notifyAll时，当前线程被唤醒，接着在wait返回时，
			// UnassignGuard的析构函数自动将mutex的holder设置为当前线程。
			m_owner.unassignHolder();
		}

		~UnassignGuard()
		{
			m_owner.assignHolder();
		}

	private:
		// 引用
		MutexLock& m_owner;
	};
	// 解除锁的持有者
	void unassignHolder()
	{
		m_holder = sz_invalid_pid;
	}
	// 设置锁的持有者
	void assignHolder()
	{
		m_holder = CurrentThread::tid();
	}

	// 互斥量
	sz_mutex_t m_mutex;
	// 由哪个线程持有
	sz_pid_t m_holder;
};

// RAII
class MutexLockGuard : NonCopyable
{
public:
	explicit MutexLockGuard(MutexLock& mutex): 
		m_mutex(mutex)
	{
		m_mutex.lock();
	}
	~MutexLockGuard()
	{
		m_mutex.unlock();
	}

private:
	MutexLock& m_mutex;
};

}  // end namespace sznet

#endif // _SZNET_THREAD_MUTEX_H_
