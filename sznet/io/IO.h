#ifndef _SZNET_IO_H_
#define _SZNET_IO_H_

#include "../NetCmn.h"
#include "stdio.h"

#if defined(SZ_OS_WINDOWS)
#	include <io.h>
#endif

// 是否是一个常规文件
/* Define S_ISREG if not defined by system headers, f.e. MSVC */
#if !defined(S_ISREG) && defined(S_IFMT) && defined(S_IFREG)
#	define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

// 是否是一个目录
/* Define S_ISDIR if not defined by system headers, f.e. MSVC */
#if !defined(S_ISDIR) && defined(S_IFMT) && defined(S_IFDIR)
#	define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

namespace sznet
{

// 打开文件流
FILE* sz_fopen(const char* name, const char* mode);
// 关闭文件流
int sz_fclose(FILE* s);
// 刷新文件流
int sz_fflush(FILE* s);
// 不加锁的写入文件流
size_t sz_fwrite_unlocked(const void* ptr, size_t size, size_t n, FILE *stream);
// 打开文件
int sz_open(const char* pathname, int flags);
// 关闭文件描述符
int sz_close(int fd);
// 读取文件描述符
int sz_read(int fd, void* buf, size_t nbyte);
// 获取文件描述符状态
int sz_fstat(int fd, struct stat* const stat);

} // end namespace sznet

#endif // _SZNET_IO_H_
