#include "CurrentThread.h"

#if SZ_OS_LINUX 
#	include <cxxabi.h>
#	include <execinfo.h>
#	include <sys/syscall.h>
#endif

#if SZ_OS_WINDOWS
#endif

#include <stdlib.h>

namespace sznet
{

namespace CurrentThread
{

thread_local sz_pid_t t_cachedTid = 0;
thread_local char t_tidString[32];
thread_local int t_tidStringLength = 6;
thread_local const char* t_threadName = "unknown";

// 缓存线程tid，写写t_cachedTid & t_tidString & t_tidStringLength
void cacheTid()
{
	if (t_cachedTid == 0)
	{
#if defined(SZ_OS_WINDOWS)
		t_cachedTid = GetCurrentThreadId();
#else
		t_cachedTid = syscall(SYS_gettid);
#endif
		t_tidStringLength = snprintf(t_tidString, sizeof(t_tidString), "%5d ", t_cachedTid);
	}
}

#if defined(SZ_OS_WINDOWS)

string stackTrace(bool demangle)
{
	(void)demangle;
	return "";
}

#else

string stackTrace(bool demangle)
{
	string stack;
	const int max_frames = 200;
	void* frame[max_frames];
	// 该函数用于获取当前线程的调用堆栈,获取的信息将会被存放在buffer中,它是一个指针列表
	int nptrs = ::backtrace(frame, max_frames);
	// backtrace_symbols将从backtrace函数获取的信息转化为一个字符串数组
	char** strings = ::backtrace_symbols(frame, nptrs);
	if (strings)
	{
		size_t len = 256;
		char* demangled = demangle ? static_cast<char*>(::malloc(len)) : nullptr;
		for (int i = 1; i < nptrs; ++i)  // skipping the 0-th, which is this function
		{
			if (demangle)
			{
				// https://panthema.net/2008/0901-stacktrace-demangled/
				// bin/exception_test(_ZN3Bar4testEv+0x79) [0x401909]
				char* left_par = nullptr;
				char* plus = nullptr;
				for (char* p = strings[i]; *p; ++p)
				{
					if (*p == '(')
						left_par = p;
					else if (*p == '+')
						plus = p;
				}

				if (left_par && plus)
				{
					*plus = '\0';
					int status = 0;
					char* ret = abi::__cxa_demangle(left_par + 1, demangled, &len, &status);
					*plus = '+';
					if (status == 0)
					{
						demangled = ret;  // ret could be realloc()
						stack.append(strings[i], left_par + 1);
						stack.append(demangled);
						stack.append(plus);
						stack.push_back('\n');
						continue;
					}
				}
			}
			// Fallback to mangled names
			stack.append(strings[i]);
			stack.push_back('\n');
		}
		free(demangled);
		free(strings);
	}
	return stack;
}

#endif

} // end namespace CurrentThread

} // end namespace sznet
