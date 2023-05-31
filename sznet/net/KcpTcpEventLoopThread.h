// Comment: KCP&TCP线程IO事件循环实现

#ifndef _SZNET_NET_KCPTCPEVENTLOOPTHREAD_H_
#define _SZNET_NET_KCPTCPEVENTLOOPTHREAD_H_

#include "../thread/Condition.h"
#include "../thread/Mutex.h"
#include "../thread/ThreadClass.h"
#include "../base/NonCopyable.h"
#include "InetAddress.h"

namespace sznet
{

namespace net
{

// KCP&TCP IO事件循环
class KcpTcpEventLoop;

// KCP&TCP线程IO事件循环
class KcpTcpEventLoopThread : NonCopyable
{
public:
	// 线程IO线程循环前的回调函数类型
	typedef std::function<void(KcpTcpEventLoop*)> ThreadInitCallback;

	KcpTcpEventLoopThread(InetAddress m_udpLisetenAddr, const ThreadInitCallback& cb = ThreadInitCallback(), const string& name = string(), int seed = 0);
	~KcpTcpEventLoopThread();

	// 开启IO事件循环
	KcpTcpEventLoop* startLoop();

private:
	// IO事件循环工作线程
	void threadFunc();

	// udp监听地址
	InetAddress m_udpLisetenAddr;
	// 指向IO事件循环对象
	KcpTcpEventLoop* m_loop;
	// 是否退出
	bool m_exiting;
	// 线程对象，用于创建m_loop的工作线程
	Thread m_thread;
	// 配合条件变量保护m_loop
	MutexLock m_mutex;
	Condition m_cond;
	// 线程IO事件循环前的回调函数
	ThreadInitCallback m_callback;
	// 随机数种子
	int m_seed;
	// 名称
	string m_name;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_KCPTCPEVENTLOOPTHREAD_H_

