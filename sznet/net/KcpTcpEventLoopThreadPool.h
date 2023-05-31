// Comment: KCP&TCP线程池事件IO循环实现

#ifndef _SZNET_NET_KCPTCPEVENTLOOPTHREADPOOL_H_
#define _SZNET_NET_KCPTCPEVENTLOOPTHREADPOOL_H_

#include "../NetCmn.h"
#include "../base/NonCopyable.h"
#include "InetAddress.h"

#include <functional>
#include <memory>
#include <vector>

namespace sznet
{

namespace net
{

// KCP&TCP事件IO循环
class KcpTcpEventLoop;
// KCP&TCP线程事件IO循环
class KcpTcpEventLoopThread;

// KCP&TCP线程池事件IO循环
class KcpTcpEventLoopThreadPool : NonCopyable
{
public:
	// 线程IO事件循环前的回调函数类型
	typedef std::function<void(KcpTcpEventLoop*)> ThreadInitCallback;

	KcpTcpEventLoopThreadPool(KcpTcpEventLoop* baseLoop, const std::vector<InetAddress>& udpListenAddrs, const string& nameArg);
	~KcpTcpEventLoopThreadPool();

	// 设置IO事件循环的线程数量
	void setThreadNum(int numThreads)
	{
		m_numThreads = numThreads;
	}
	// 启动线程池
	void start(const ThreadInitCallback& cb = ThreadInitCallback());

	// 轮询获取事件IO循环
	KcpTcpEventLoop* getNextLoop();
	// 和线城池容量进行hash获取事件IO循环
	KcpTcpEventLoop* getLoopForHash(size_t hashCode);
	// 获取所有事件IO循环对象
	std::vector<KcpTcpEventLoop*> getAllLoops();
	// 返回是否启动
	bool started() const
	{
		return m_started;
	}
	// 返回名称前缀
	const string& name() const
	{
		return m_name;
	}

private:
	// acceptor所属的事件IO循环
	KcpTcpEventLoop* m_baseLoop;
	// 线程名称前缀
	string m_name;
	// 是否启动
	bool m_started;
	// 线程数量
	int m_numThreads;
	// 新连接到来，所选择的EventLoop对象下标
	int m_next;
	// 线程事件IO循环指针数组
	std::vector<std::unique_ptr<KcpTcpEventLoopThread>> m_threads;
	// 事件IO循环指针数组
	std::vector<KcpTcpEventLoop*> m_loops;
	// UDP多个地址，和线程数量必须是一致
	const std::vector<InetAddress> m_udpListenAddrs;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_KCPTCPEVENTLOOPTHREADPOOL_H_
