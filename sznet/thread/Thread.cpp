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

#endif

} // namespace sznet