#include "Thread.h"

namespace sznet
{

#if defined(SZ_OS_WINDOWS)

void sz_mutex_init(sz_mutex_t* mutex)
{
	InitializeCriticalSection(mutex);
}

void sz_mutex_fini(sz_mutex_t* mutex)
{
	DeleteCriticalSection(mutex);
}

void sz_mutex_lock(sz_mutex_t* mutex)
{
	EnterCriticalSection(mutex);
}

void sz_mutex_unlock(sz_mutex_t* mutex)
{
	LeaveCriticalSection(mutex);
}

sz_thread_t	sz_thread_create(xd_thread_func_t func, void* param)
{
	return (sz_thread_t)_beginthreadex(nullptr, 0, func, param, 0, nullptr);
}

int	sz_waitfor_thread_terminate(sz_thread_t thread)
{
	if (WaitForSingleObject(thread, INFINITE) != WAIT_OBJECT_0)
	{
		return -1;
	}

	CloseHandle(thread);
	return 0;
}

void sz_thread_detach(sz_thread_t thread)
{
	CloseHandle(thread);
}

void sz_cond_init(sz_cond_t* cond)
{
	*cond = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void sz_cond_fini(sz_cond_t* cond)
{
	CloseHandle(*cond);
}

void sz_cond_notify(sz_cond_t* cond)
{
	SetEvent(*cond);
}

void sz_cond_notifyall(sz_cond_t* cond)
{
	PulseEvent(*cond);
}

int sz_cond_wait(sz_cond_t* cond, sz_mutex_t* mutex)
{
	EnterCriticalSection(mutex);

	auto rst = WaitForSingleObject(*cond, INFINITE) == WAIT_OBJECT_0 ? 0 : -1;
	
	LeaveCriticalSection(mutex);

	return rst;
}

int sz_cond_timewait(sz_cond_t* cond, sz_mutex_t* mutex, int& millisec)
{
	EnterCriticalSection(mutex);

	auto rst = WaitForSingleObject(*cond, millisec) == WAIT_OBJECT_0 ? 0 : -1;

	LeaveCriticalSection(mutex);

	return rst;
}

#elif defined(SZ_OS_LINUX)

void sz_mutex_init(sz_mutex_t* mutex)
{
	pthread_mutex_init(mutex, nullptr);
}

void sz_mutex_fini(sz_mutex_t* mutex)
{
	pthread_mutex_destroy(mutex);
}
	
void sz_mutex_lock(sz_mutex_t* mutex)
{
	pthread_mutex_lock(mutex);
}
void sz_mutex_unlock(sz_mutex_t* mutex)
{
	pthread_mutex_unlock(mutex);
}

sz_thread_t	sz_thread_create(xd_thread_func_t func, void* param)
{
	sz_thread_t thread;
	if (pthread_create(&thread, nullptr, func, param) != 0)
	{
		return -1;
	}

	return thread;
}

int	sz_waitfor_thread_terminate(sz_thread_t thread)
{
	return pthread_join(thread, nullptr);
}

void sz_thread_detach(sz_thread_t thread)
{
	pthread_detach(thread);
}

void sz_cond_init(sz_cond_t* cond)
{
	pthread_cond_init(cond, nullptr);
}

void sz_cond_fini(sz_cond_t* cond)
{
	pthread_cond_destroy(cond);
}

void sz_cond_notify(sz_cond_t* cond)
{
	pthread_cond_signal(cond);
}

void sz_cond_notifyall(sz_cond_t* cond)
{
	pthread_cond_broadcast(cond);
}

int sz_cond_wait(sz_cond_t* cond, sz_mutex_t* mutex)
{
	if (pthread_mutex_lock(mutex) != 0)
	{
		return -2;
	}

	auto rst = pthread_cond_wait(event, mutex) == 0 ? 0 : -1;

	pthread_mutex_unlock(mutex);

	return rst;
}

int sz_cond_timewait(sz_cond_t* cond, sz_mutex_t* mutex, int& millisec)
{
	if (pthread_mutex_lock(mutex) != 0)
	{
		return -2;
	}

	struct timespec abstime;
	clock_gettime(CLOCK_REALTIME, &abstime);

	const int64_t kNanoSecondsPerSecond = 1000000000;
	int64_t nanoseconds = static_cast<int64_t>(millisec * (kNanoSecondsPerSecond / 1000));

	abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoseconds) / kNanoSecondsPerSecond);
	abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);
	rst = pthread_cond_timedwait(event, mutex, &abstime) == 0 ? 0 : -1;

	pthread_mutex_unlock(mutex);

	return rst;
}

#endif

} // namespace sznet