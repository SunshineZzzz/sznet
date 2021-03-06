﻿#include "Process.h"

namespace sznet
{

string sz_gethostname()
{
	char buf[256];
	if (::gethostname(buf, sizeof(buf)) == 0)
	{
		buf[sizeof(buf) - 1] = '\0';
		return buf;
	}
	else
	{
		return "unknownhost";
	}
}

int sz_getlasterr()
{
#ifdef SZ_OS_LINUX
	return errno;
#endif
#ifdef SZ_OS_WINDOWS
	return WSAGetLastError();
#endif
}

#if defined(SZ_OS_WINDOWS)

sz_pid_t sz_process_getpid()
{
	return GetCurrentProcessId();
}

#elif defined(SZ_OS_LINUX)

sz_pid_t sz_process_getpid()
{
	return getpid();
}

#endif

} // end namespace sznet