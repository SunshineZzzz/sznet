// Comment: KcpWithTcp客户端实现

#ifndef _SZNET_NET_KCPWITHTCPCLIENT_H_
#define _SZNET_NET_KCPWITHTCPCLIENT_H_

#include "../thread/Mutex.h"
#include "../time/TimerId.h"
#include "TcpConnection.h"
#include "KcpConnection.h"
#include "Codec.h"
#include "InetAddress.h"
#include "Socket.h"
#include "EventLoop.h"
#include "Channel.h"

#include <atomic>

namespace sznet
{

namespace net
{

// 主动连接实现
class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

// KCP客户端
class KcpWithTcpClient : NonCopyable
{
public:
	KcpWithTcpClient(EventLoop* loop, const InetAddress& tcpServerAddr, const string& nameArg);
	// force out-line dtor, for std::unique_ptr members.
	~KcpWithTcpClient();

	// 发起TCP连接
	void connectTcp();
	// 关闭TCP连接
	void disconnectTcp();
	// 停止TCP重连
	void stopTcp();
	// KCP是否连接
	bool kcpConnected() const
	{
		MutexLockGuard lock(m_mutex);
		if (m_kcpConnection)
		{
			return m_kcpConnection->connected();
		}
		return false;
	}
	// 返回KCP连接
	KcpConnectionPtr kcpConnection() const
	{
		MutexLockGuard lock(m_mutex);
		return m_kcpConnection;
	}
	// 返回loop
	EventLoop* getLoop() const
	{ 
		return m_loop; 
	}
	// 是否可以重连
	bool tcpRetry() const
	{ 
		return m_tcpRetry; 
	}
	// 允许重连
	void enableTcpRetry() 
	{ 
		m_tcpRetry = true;
	}
	// 该连接的名称
	const string& name() const
	{ 
		return m_name; 
	}
	// 设置KCP连接建立/断开回调函数
	/// Set connection callback.
	/// Not thread safe.
	void setKcpConnectionCallback(KcpConnectionCallback cb)
	{ 
		m_kcpConnectionCallback = std::move(cb); 
	}
	// 设置KCP处理收到消息的回调调函数
	/// Set message callback.
	/// Not thread safe.
	void setKcpMessageCallback(KcpMessageCallback cb)
	{ 
		m_kcpMessageCallback = std::move(cb); 
	}
	// 设置KCP发送缓冲区清空回调函数
	/// Set write complete callback.
	/// Not thread safe.
	void setKcpWriteCompleteCallback(KcpWriteCompleteCallback cb)
	{ 
		m_kcpWriteCompleteCallback = std::move(cb); 
	}

private:
	// TCP连接成功回调
	/// Not thread safe, but in loop
	void newTcpConnection(sockets::sz_sock sockfd);
	// TCP连接断开回调
	/// Not thread safe, but in loop
	void removeTcpConnection(const TcpConnectionPtr& tcpConn);
	// KCP连接断开回调
	void removeKcpConnection(const KcpConnectionPtr& kcpConn);
	// TCP消息回调
	void handleTcpMessage(const TcpConnectionPtr& tcpConn, const string& message, Timestamp receiveTime);

	// 连接绑定的loop
	EventLoop* m_loop;
	// avoid revealing Connector
	// 主动连接
	ConnectorPtr m_tcpConnector; 
	// 配合m_nextConnId，生成连接名称
	const string m_name;
	// KCP连接建立/断开回调函数
	KcpConnectionCallback m_kcpConnectionCallback;
	// KCP处理收到消息的回调调函数
	KcpMessageCallback m_kcpMessageCallback;
	// KCP发送缓冲区清空回调函数
	KcpWriteCompleteCallback m_kcpWriteCompleteCallback;
	// 默认false，手动调用 enableRetry 设置为true
	std::atomic<bool> m_tcpRetry;
	// 默认true，主动断开，设置 false
	// 非主动断掉以后，这两都为true才会调用 m_connector->restart()
	std::atomic<bool> m_tcpConnect;
	// 连接成功自增ID
	// always in loop thread
	uint32_t m_nextConnId;
	// 互斥量
	mutable MutexLock m_mutex;
	// 一个 shared_ptr 对象实体可被多个线程同时读取
	// 两个 shared_ptr 对象实体可以被两个线程同时写入，“析构”算写操作
	// 如果要从多个线程读写同一个 shared_ptr 对象，那么需要加锁
	// TCP连接
	TcpConnectionPtr m_tcpConnection;
	// 4字节编解码器
	LengthHeaderCodec<> m_tcpCodec;
	// UDP 监听Addr
	// 初始化的时候用的是TCP的监听地址
	// 拿到KCP端口以后再进行修改
	InetAddress m_udpListenAddr;
	// KCP连接
	KcpConnectionPtr m_kcpConnection;
};

} // namespace net

} // namespace sznet

#endif // _SZNET_NET_KCPWITHTCPCLIENT_H_
