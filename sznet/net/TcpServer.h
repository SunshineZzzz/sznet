// Comment: TCP服务器实现

#ifndef _SZNET_NET_TCPSERVER_H_
#define _SZNET_NET_TCPSERVER_H_

#include "../NetCmn.h"
#include "TcpConnection.h"

#include <atomic>
#include <map>

namespace sznet
{

namespace net
{

// 连接接收器
class Acceptor;
// 事件IO循环
class EventLoop;
// 事件IO循环线程池
class EventLoopThreadPool;

// TCP服务器，支持单线程和线程池
class TcpServer : NonCopyable
{
public:
	// 线程IO事件循环前的回调函数类型
	typedef std::function<void(EventLoop*)> ThreadInitCallback;
	// 初始化m_acceptor的配置项
	enum Option
	{
		// 不重用端口
		kNoReusePort,
		// 重用端口
		kReusePort,
	};

	TcpServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg, Option option = kNoReusePort);
	~TcpServer();

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
	EventLoop* getLoop() const 
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
	std::shared_ptr<EventLoopThreadPool> threadPool()
	{
		return m_threadPool;
	}
	// 启动Server，监听客户连接
	// 可以多次调用，线程安全
	void start();
	// 设置外部连接/断开通知回调函数
	/// Set connection callback.
	/// Not thread safe.
	void setConnectionCallback(const ConnectionCallback& cb)
	{
		m_connectionCallback = cb;
	}
	// 外部处理收到消息的回调调函数
	/// Set message callback.
	/// Not thread safe.
	void setMessageCallback(const MessageCallback& cb)
	{
		m_messageCallback = cb;
	}
	// 设置数据发送完成的回调函数，直接发送成功/发送缓冲区发送完毕
	/// Set write complete callback.
	/// Not thread safe.
	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{
		m_writeCompleteCallback = cb;
	}

private:
	// 新连接来了
	// 只能在accept io loop中都调用，非线程安全
	void newConnection(sockets::sz_sock sockfd, const InetAddress& peerAddr);
	// 移除指定的连接
	// 线程安全
	void removeConnection(const TcpConnectionPtr& conn);
	// 移除指定连接
	// 只能在accept io loop中都调用，非线程安全
	void removeConnectionInLoop(const TcpConnectionPtr& conn);
	// 连接管理器类型
	typedef std::map<uint32_t, TcpConnectionPtr> ConnectionMap;

	// 接受新连接的线程，而新连接会用线程池返回的subReactor EventLoop来执行IO
	EventLoop* m_loop;
	// 监听地址端口字符串
	const string m_ipPort;
	// 服务器名称
	const string m_name;
	// 连接接收器，使用该类来创建，监听连接，并通过处理该套接字来获得新连接sockfd
	std::unique_ptr<Acceptor> m_acceptor;
	// 线程池IO事件循环
	std::shared_ptr<EventLoopThreadPool> m_threadPool;
	// TCP连接/断开通知回调函数
	ConnectionCallback m_connectionCallback;
	// TCP处理收到消息的回调调函数
	MessageCallback m_messageCallback;
	// TCP数据发送完成的回调函数，直接发送成功/发送缓冲区发送完毕
	WriteCompleteCallback m_writeCompleteCallback;
	// IO事件循环前的回调函数
	ThreadInitCallback m_threadInitCallback;
	// 用于只启动一次服务器
	std::atomic<int> m_started;
	// 生成连接ID
	uint32_t m_nextConnId;
	// 连接管理器
	// 连接名称 <-> 连接对象的智能指针
	ConnectionMap m_connections;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_TCPSERVER_H_
