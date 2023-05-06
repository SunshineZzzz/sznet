// Comment: 公共头文件

#ifndef _SZNET_NETCMN_H_
#define _SZNET_NETCMN_H_

#include "base/Platform.h"
#include "base/Compiler.h"
#include "base/Types.h"

#include <inttypes.h>

#if SZ_OS_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#		define NOGDI
#		define NOMINMAX 
#			include <Windows.h>
#			include <WinSock2.h>
#			pragma comment(lib, "ws2_32.lib")
#		undef NOMINMAX
#		undef NOGDI
#	undef WIN32_LEAN_AND_MEAN
#	include <crtdbg.h>
#endif

#if SZ_OS_LINUX
#	include <unistd.h>
#	include <sys/types.h>
	using sz_ssize_t = ssize_t;
#endif

// 分支预测
#if SZ_COMPILER_MSVC
#	define expect(expr,value) (expr)
	using sz_ssize_t = int;
#endif

#if SZ_COMPILER_GNUC 
#	if defined(__GNUC__) && (__GNUC__ >= 3)
#		define expect(expr,value) (__builtin_expect ((expr),(value)))
#	else
#		define expect(expr,value) (expr)
#	endif
#endif

// 分支预测
#ifndef likely
#	define likely(expr) expect((expr) != 0, 1)
#endif
#ifndef unlikely
#	define unlikely(expr) expect((expr) != 0, 0)
#endif

namespace sznet
{

// 安全的delete
template<class _Ty>
void sz_safedelete(_Ty*& pT)
{
	if (pT)
	{
		delete pT;
		pT = nullptr;
	}
}
// 安全的delete[] 
template<class _Ty>
void sz_safedeletearray(_Ty*& pT)
{
	if (pT)
	{
		delete[] pT;
		pT = nullptr;
	}
}

// windows下内存泄漏报警
void sztool_memcheck();
void sztool_setbreakalloc(int size);

} // end namespace sznet

namespace
{

// 初始化网络环境
class InitSocket
{
public:
	InitSocket()
	{
#if defined (SZ_OS_WINDOWS)
		WSADATA WS;
		if (WSAStartup(MAKEWORD(2, 2), &WS) != 0)
		{
			puts("WSAStartup failed!");
			abort();
		}
#endif
	}
	~InitSocket()
	{
#if defined (SZ_OS_WINDOWS)
		WSACleanup();
#endif
	}
};
InitSocket initWSA;

} // end namespace

#endif // _SZNET_NETCMN_H_