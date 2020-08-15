#ifndef _SZNET_TIME_TIME_H_
#define _SZNET_TIME_TIME_H_

#include "../NetCmn.h"
#include <time.h>

#if defined(SZ_OS_WINDOWS)
#	include <WinSock2.h>
#endif

#if defined(SZ_OS_LINUX)
#	include <sys/time.h>
#endif

namespace sznet
{

// 线程安全的localtime
void sz_localtime(struct tm& t, time_t n = -1);
// 返回微妙时间戳
int sz_gettimeofday(struct timeval* tv);
// 休眠毫秒
void sz_sleep(int millisec);

} // namespace sznet

#endif // _SZNET_TIME_TIME_H_