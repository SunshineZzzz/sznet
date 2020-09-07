#ifndef _SZNET_THREAD_THREAD_H_
#define _SZNET_THREAD_THREAD_H_

#include "../NetCmn.h"

#if SZ_OS_LINUX 
#	include <pthread.h>
#endif

#ifdef SZ_OS_WINDOWS
#	include <process.h>
#endif

namespace sznet
{

#ifdef SZ_OS_LINUX
	using sz_mutex_t = pthread_mutex_t;
	using sz_thread_t = pthread_t;
	using sz_cond_t = pthread_cond_t;
#	define sz_invalid_thread 0
#	define sz_thread_func_return void*
	using xd_thread_func_t = sz_thread_func_return(*)(void*);
#endif

#ifdef SZ_OS_WINDOWS
	using sz_mutex_t = CRITICAL_SECTION;
	using sz_thread_t = HANDLE;
	using sz_cond_t = CONDITION_VARIABLE;
#	define sz_invalid_thread nullptr
#	define sz_thread_func_return unsigned int
	using xd_thread_func_t = sz_thread_func_return(__stdcall *)(void*);
#endif

// 互斥量初始化
void sz_mutex_init(sz_mutex_t* mutex);
// 互斥量释放
void sz_mutex_fini(sz_mutex_t* mutex);
// 互斥量加锁
void sz_mutex_lock(sz_mutex_t* mutex);
// 互斥量解锁
void sz_mutex_unlock(sz_mutex_t* mutex);
// 创建线程
sz_thread_t	sz_thread_create(xd_thread_func_t func, void* param);
// 等待线程结束
int	sz_waitfor_thread_terminate(sz_thread_t thread);
// 线程detach
void sz_thread_detach(sz_thread_t thread);
// 条件变量初始化
void sz_cond_init(sz_cond_t* cond);
// 条件变量結束
void sz_cond_fini(sz_cond_t* cond);
// 唤醒等待在相应条件变量上的一个线程
void sz_cond_notify(sz_cond_t* cond);
// 唤醒阻塞在相应条件变量上的所有线程
void sz_cond_notifyall(sz_cond_t* cond);
// 等待条件变量被触发
int sz_cond_wait(sz_cond_t* cond, sz_mutex_t* mutex);
// 限时等待条件变量被触发
int sz_cond_timewait(sz_cond_t* cond, sz_mutex_t* mutex, uint32_t millisec);

}  // end namespace sznet

#endif // _SZNET_THREAD_THREAD_H_