// Comment: TCP客户端实现

#ifndef _SZNET_NET_TCPCLIENT_H_
#define _SZNET_NET_TCPCLIENT_H_

#include "../thread/Mutex.h"
#include "TcpConnection.h"

namespace sznet
{
namespace net
{

// 主动连接实现
class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

// TCP客户端
class TcpClient : NonCopyable
{
public:
	// TcpClient(EventLoop* loop);
	// TcpClient(EventLoop* loop, const string& host, uint16_t port);
	TcpClient(EventLoop* loop, const InetAddress& serverAddr, const string& nameArg);
	// force out-line dtor, for std::unique_ptr members.
	~TcpClient();

	// 发起连接
	void connect();
	// 半关闭，关闭写端
	void disconnect();
	// 停止重连
	void stop();
	// 返回TCP连接
	TcpConnectionPtr connection() const
	{
		MutexLockGuard lock(m_mutex);
		return m_connection;
	}
	// 返回loop
	EventLoop* getLoop() const 
	{ 
		return m_loop; 
	}
	// 是否可以重连
	bool retry() const
	{ 
		return m_retry; 
	}
	// 允许重连
	void enableRetry() 
	{ 
		m_retry = true; 
	}
	// 该连接的名称
	const string& name() const
	{ 
		return m_name; 
	}
	// 设置外部连接建立/断开回调函数
	/// Set connection callback.
	/// Not thread safe.
	void setConnectionCallback(ConnectionCallback cb)
	{ 
		m_connectionCallback = std::move(cb); 
	}
	// 设置外部处理收到消息的回调调函数
	/// Set message callback.
	/// Not thread safe.
	void setMessageCallback(MessageCallback cb)
	{ 
		m_messageCallback = std::move(cb); 
	}
	// 设置发送缓冲区清空回调函数
	/// Set write complete callback.
	/// Not thread safe.
	void setWriteCompleteCallback(WriteCompleteCallback cb)
	{ 
		m_writeCompleteCallback = std::move(cb); 
	}

private:
	// 连接成功
	/// Not thread safe, but in loop
	void newConnection(sockets::sz_sock sockfd);
	// 连接断开
	/// Not thread safe, but in loop
	void removeConnection(const TcpConnectionPtr& conn);

	// 连接绑定的loop
	EventLoop* m_loop;
	// avoid revealing Connector
	// 主动连接
	ConnectorPtr m_connector; 
	// 配合m_nextConnId，生成连接名称
	const string m_name;
	// 外部连接建立/断开回调函数
	ConnectionCallback m_connectionCallback;
	// 外部处理收到消息的回调调函数
	MessageCallback m_messageCallback;
	// 发送缓冲区清空回调函数
	WriteCompleteCallback m_writeCompleteCallback;
	// 默认false，手动调用 enableRetry 设置为true
	// atomic
	bool m_retry;
	// 默认true，主动断开，设置 false
	// atomic
	bool m_connect;
	// 连接成功自增ID
	// always in loop thread
	int m_nextConnId;
	// 互斥量
	mutable MutexLock m_mutex;
	// 一个 shared_ptr 对象实体可被多个线程同时读取
	// 两个 shared_ptr 对象实体可以被两个线程同时写入，“析构”算写操作
	// 如果要从多个线程读写同一个 shared_ptr 对象，那么需要加锁
	// tcp连接
	TcpConnectionPtr m_connection;
};

} // namespace net

} // namespace sznet

#endif // _SZNET_NET_TCPCLIENT_H_
