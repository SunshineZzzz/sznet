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

// ��������ʼ��
void sz_mutex_init(sz_mutex_t* mutex);
// �������ͷ�
void sz_mutex_fini(sz_mutex_t* mutex);
// ����������
void sz_mutex_lock(sz_mutex_t* mutex);
// ����������
void sz_mutex_unlock(sz_mutex_t* mutex);

}  // namespace sznet

#endif // _SZNET_THREAD_THREAD_H_