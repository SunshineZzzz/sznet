#ifndef _SZNET_THREAD_THREAD_H_
#define _SZNET_THREAD_THREAD_H_

#include "../NetCmn.h"

namespace sznet
{
#ifdef SZ_OS_LINUX
	using sz_mutex_t = pthread_mutex_t;
#endif

#ifdef SZ_OS_WINDOWS
	using sz_mutex_t = CRITICAL_SECTION;
#endif

// »¥³âÁ¿³õÊ¼»¯
void sz_mutex_init(sz_mutex_t* mutex);
// »¥³âÁ¿ÊÍ·Å
void sz_mutex_fini(sz_mutex_t* mutex);
// »¥³âÁ¿¼ÓËø
void sz_mutex_lock(sz_mutex_t* mutex);
// »¥³âÁ¿½âËø
void sz_mutex_unlock(sz_mutex_t* mutex);

}  // namespace sznet

#endif // _SZNET_THREAD_THREAD_H_