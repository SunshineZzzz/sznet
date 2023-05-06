// Comment: 进程相关函数接口

#ifndef _SZ_PROCESS_H_
#define _SZ_PROCESS_H_

#include "../NetCmn.h"

namespace sznet
{

#ifdef SZ_OS_LINUX
	using sz_pid_t = pid_t;
#endif

#ifdef SZ_OS_WINDOWS
	using sz_pid_t = DWORD;
#endif

#define sz_invalid_pid 0

// 获得进程号
sz_pid_t sz_getpid();
// 获取主机名称
string sz_gethostname();
// 返回上一次错误
int sz_getlasterr();
// 线程安全根据errno获取错误字符串
const char* sz_strerror_tl(int savedErrno);

} // end namespace sznet

#endif // _SZ_PROCESS_H_
