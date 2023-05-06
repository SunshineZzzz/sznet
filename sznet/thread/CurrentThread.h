// Comment: 当前线程信息

#ifndef _SZNET_THREAD_CURRENTTHREAD_H_
#define _SZNET_THREAD_CURRENTTHREAD_H_

#include "../NetCmn.h"
#include "../process/Process.h"
#include <thread>

namespace sznet
{

namespace CurrentThread
{

// internal
// 线程tid的缓存，避免每次都调用(syscall(SYS_gettid))获取，提高获取tid的效率
// Linux下的POSIX线程也有一个id，类型pthread_t，由pthread_self()取得，
// 该id由线程库维护，其id空间是各个进程独立的(即不同进程中的线程可能有相同的id)。
extern thread_local sz_pid_t t_cachedTid;
// 线程tid的字符串表示形式
extern thread_local char t_tidString[32];
// 线程tid的字符串的长度
extern thread_local int t_tidStringLength;
// 自定义线程名称，并且可以标记线程状态(finished, crashed)
extern thread_local const char* t_threadName;
// 缓存线程tid，写 t_cachedTid & t_tidString & t_tidStringLength
void cacheTid();

// 获取当前线程tid
inline sz_pid_t tid()
{
	// 分支预测
	if (unlikely(t_cachedTid == 0))
	{
		// 进入这里的可能性很小
		cacheTid();
	}
	return t_cachedTid;
}

// 返回当前线程tid
// for logging
inline const char* tidString()
{
	return t_tidString;
}

// 返回当前线程tid的长度
// for logging
inline int tidStringLength()
{
	return t_tidStringLength;
}

// 返回当前线程名称
inline const char* name()
{
	return t_threadName;
}

// 当前线程是否是主线程
bool isMainThread();

// 堆栈信息，demangle是否解析mangle
string stackTrace(bool demangle);

} // end namespace CurrentThread

} // end namespace sznet

#endif // _SZNET_THREAD_CURRENTTHREAD_H_
