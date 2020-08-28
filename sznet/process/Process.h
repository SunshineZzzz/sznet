#ifndef _SZ_PROCESS_H_
#define _SZ_PROCESS_H_

#include "../NetCmn.h"

#ifdef SZ_OS_WINDOWS
#	include <WinSock2.h>
#endif

namespace sznet
{

#ifdef SZ_OS_LINUX
	using sz_pid_t = pid_t;
#endif
#ifdef SZ_OS_WINDOWS
	using sz_pid_t = DWORD	;
#endif
#define sz_invalid_pid 0

// 获得进程号
sz_pid_t sz_process_getpid();
// 获取主机名称
string sz_gethostname();
// 返回上一次错误
int sz_getlasterr();

} // end namespace sznet

#endif // _SZ_PROCESS_H_
