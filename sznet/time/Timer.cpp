#include "Time.h"

namespace sznet
{

void sz_localtime(struct tm& t, time_t n)
{
	time_t lt;
	if (n == -1)
	{
		lt = time(nullptr);
	}
	else
	{
		lt = n;
	}
#if SZ_OS_LINUX || SZ_OS_CYGWIN
	localtime_r(&lt, &t);
#elif SZ_OS_WINDOWS
	t = *(localtime(&lt));
#endif
}

void sz_sleep(int millisec)
{
#if SZ_OS_LINUX
	struct timespec ts {};
	ts.tv_sec = millisec / 1000;
	ts.tv_nsec = (millisec % 1000) * 1000000;
	nanosleep(&ts, nullptr);
#elif SZ_OS_WINDOWS
	Sleep(millisec);
#endif
}

#if defined(SZ_OS_WINDOWS)

// From https://gist.github.com/ugovaretto/5875385#file-win-gettimeofday-c
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000Ui64
int sz_gettimeofday(struct timeval* tv)
{
	FILETIME ft;
	unsigned __int64 tmpres = 0;

	if (nullptr != tv)
	{
		GetSystemTimeAsFileTime(&ft);

		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		tmpres /= 10;   /*convert into microseconds*/
						/*converting file time to unix epoch*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS;
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

	return 0;
}

#elif defined(SZ_OS_LINUX)

int sz_gettimeofday(struct timeval *tv)
{
	return gettimeofday(tv, nullptr);
}

#endif

} // namespace sznet