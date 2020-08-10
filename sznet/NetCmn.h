#ifndef _SZNET_NETCMN_H_
#define _SZNET_NETCMN_H_

#include "base/Platform.h"
#include "base/Compiler.h"

#include <inttypes.h>  

#if SZ_OS_WINDOWS
#	include <Windows.h>
#	include <crtdbg.h>
#endif

#if SZ_COMPILER_GNUC 
#	include <pthread.h>
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

} // namespace sznet

#endif // _SZNET_NETCMN_H_