#ifndef _SZNET_THREAD_CONDITION_H_
#define _SZNET_THREAD_CONDITION_H_

#include "../base/NonCopyable.h"
#include "Mutex.h"

namespace sznet
{

// 条件变量
class Condition : NonCopyable
{
public:
	explicit Condition(MutexLock& mutex): 
		m_mutex(mutex)
	{
		sz_cond_init(&m_cond);
	}
	~Condition()
	{
		sz_cond_fini(&m_cond);
	}

	// 等待条件变量被触发
	void wait()
	{
		MutexLock::UnassignGuard ug(m_mutex);
		sz_cond_wait(&m_cond, m_mutex.getPthreadMutex());
	}
	// 限时等待条件变量被触发
	bool waitForSeconds(int millisec)
	{
		MutexLock::UnassignGuard ug(m_mutex);
		return sz_cond_timewait(&m_cond, m_mutex.getPthreadMutex(), millisec);
	}
	// 唤醒等待在相应条件变量上的一个线程
	void notify()
	{
		sz_cond_notify(&m_cond);
	}
	// 唤醒阻塞在相应条件变量上的所有线程
	void notifyAll()
	{
		sz_cond_notifyall(&m_cond);
	}

private:
	// 互斥量
	MutexLock& m_mutex;
	// 条件变量
	sz_cond_t m_cond;
};

} // end namespace sznet

#endif // _SZNET_THREAD_CONDITION_H_
