#include "Process.h"

#include <thread>

#include <string.h>

namespace sznet
{

// 根据errno转换的错误字符串
thread_local char t_errnobuf[512];

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
#else
	return WSAGetLastError();
#endif
}

const char* sz_strerror_tl(int savedErrno)
{
#if defined(SZ_OS_WINDOWS)
	// strerror_s(t_errnobuf, sizeof(t_errnobuf), savedErrno);
	auto chars = FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		savedErrno,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)t_errnobuf,
		sizeof(t_errnobuf) - 1,
		nullptr);
	if (chars != 0)
	{
		if (chars >= 2 && t_errnobuf[chars - 2] == '\r' && t_errnobuf[chars - 1] == '\n')
		{
			chars -= 2;
			t_errnobuf[chars] = 0;
		}
	}
#else
	strerror_r(savedErrno, t_errnobuf, sizeof(t_errnobuf));
#endif
	return t_errnobuf;
}

#if defined(SZ_OS_WINDOWS)

sz_pid_t sz_getpid()
{
	return GetCurrentProcessId();
}

#else

sz_pid_t sz_getpid()
{
	return getpid();
}

#endif

} // end namespace sznet