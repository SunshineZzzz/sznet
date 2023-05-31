// Comment: Kcp Cooperate With Tcp Server

#ifndef _SZNET_NET_KCPWITHTCPSERVER_H_
#define _SZNET_NET_KCPWITHTCPSERVER_H_

#include "../NetCmn.h"
#include "KcpConnection.h"
#include "Codec.h"

#include <atomic>
#include <vector>
#include <map>
#include <random>

namespace sznet
{

namespace net
{

// 连接接收器
class Acceptor;
// KCP&TCP事件IO循环
class KcpTcpEventLoop;
// KCP&TCP事件IO循环线程池
class KcpTcpEventLoopThreadPool;

// 已有TCP服务使用KCP服务，具体请看下面链接：
// https://github.com/skywind3000/kcp/wiki/Cooperate-With-Tcp-Server
class KcpWithTcpServer : NonCopyable
{
public:
	// 线程IO事件循环前的回调函数类型
	typedef std::function<void(KcpTcpEventLoop*)> ThreadInitCallback;
	// 初始化m_acceptor的配置项
	enum Option
	{
		// 不重用端口
		kNoReusePort,
		// 重用端口
		kReusePort,
	};

	KcpWithTcpServer(KcpTcpEventLoop* loop, const InetAddress& listenAddr, const std::vector<InetAddress>& udpListenAddrs, const string& nameArg, Option option = kNoReusePort);
	~KcpWithTcpServer();

	// 监听ip&port字符串
	const string& ipPort() const
	{
		return m_ipPort;
	}
	// 获取服务器名称
	const string& name() const
	{
		return m_name;
	}
	// 获取监听loop
	KcpTcpEventLoop* getLoop() const
	{
		return m_loop;
	}
	// 设置server中需要运行多少个loop线程
	void setThreadNum(int numThreads);
	// 设置IO事件循环前的回调函数
	void setThreadInitCallback(const ThreadInitCallback& cb)
	{
		m_threadInitCallback = cb;
	}
	// 获取线程池IO事件循环
	// valid after calling start()
	std::shared_ptr<KcpTcpEventLoopThreadPool> threadPool()
	{
		return m_threadPool;
	}
	// 启动Server，监听客户连接
	// 可以多次调用，线程安全
	void start();
	// 设置KCP连接/断开通知回调函数
	/// Set connection callback.
	/// Not thread safe.
	void setKcpConnectionCallback(const KcpConnectionCallback& cb)
	{
		m_kcpConnectionCallback = cb;
	}
	// KCP处理收到消息的回调调函数
	/// Set message callback.
	/// Not thread safe.
	void setKcpMessageCallback(const KcpMessageCallback& cb)
	{
		m_kcpMessageCallback = cb;
	}
	// 设置KCP数据发送完成的回调函数，直接发送成功/发送缓冲区发送完毕
	/// Set write complete callback.
	/// Not thread safe.
	void setKcpWriteCompleteCallback(const KcpWriteCompleteCallback& cb)
	{
		m_kcpWriteCompleteCallback = cb;
	}

private:
	// 连接管理器类型
	typedef std::map<uint32_t, TcpConnectionPtr> ConnectionMap;

	// 新连接来了
	// 只能在accept io loop中都调用，非线程安全
	void newConnection(sockets::sz_sock sockfd, const InetAddress& peerAddr);
	// 移除指定的连接
	// 线程安全
	void removeConnection(const TcpConnectionPtr& conn);
	// 移除指定连接
	// 只能在accept io loop中都调用，非线程安全
	void removeConnectionInLoop(const TcpConnectionPtr& conn);
	// tcp新的连接 或者 断开连接
	void tcpConnection(const TcpConnectionPtr& conn, KcpTcpEventLoop* pKcpTcploop);
	// tcp消息，理论上不会收到任何消息，LengthHeaderCodec需要这个初始化
	void tcpMessage(const sznet::net::TcpConnectionPtr& conn, const sznet::string& message, sznet::Timestamp receiveTime);
	// 主要用来设置KCP的回调
	void onThreadInitCallback(KcpTcpEventLoop* loop);

	// 接受新连接的线程，而新连接会用线程池返回的subReactor EventLoop来执行IO
	KcpTcpEventLoop* m_loop;
	// 监听地址端口字符串
	const string m_ipPort;
	// 服务器名称
	const string m_name;
	// 连接接收器，使用该类来创建，监听连接，并通过处理该套接字来获得新连接sockfd
	std::unique_ptr<Acceptor> m_acceptor;
	// 线程池IO事件循环
	std::shared_ptr<KcpTcpEventLoopThreadPool> m_threadPool;
	// Kcp连接/断开通知回调函数
	KcpConnectionCallback m_kcpConnectionCallback;
	// KCP处理收到消息的回调调函数
	KcpMessageCallback m_kcpMessageCallback;
	// TCP数据发送完成的回调函数，直接发送成功/发送缓冲区发送完毕
	KcpWriteCompleteCallback m_kcpWriteCompleteCallback;
	// IO事件循环前的回调函数
	ThreadInitCallback m_threadInitCallback;
	// 用于只启动一次服务器
	std::atomic<int> m_started;
	// 生成连接ID
	uint32_t m_nextConnId;
	// 连接管理器
	// 连接ID <-> 连接对象的智能指针
	ConnectionMap m_connections;
	// UDP多个地址，和线程数量必须是一致
	std::vector<InetAddress> m_udpListenAddrs;
	// 4字节编解码器
	LengthHeaderCodec<> m_tcpCodec;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_KCPWITHTCPSERVER_H_
