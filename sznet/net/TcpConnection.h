// Comment: TCP连接实现

#ifndef _SZNET_NET_TCPCONNECTION_H_
#define _SZNET_NET_TCPCONNECTION_H_

#include "../NetCmn.h"
#include "../base/NonCopyable.h"
#include "../string/StringPiece.h"
#include "../base/Callbacks.h"
#include "../time/Timestamp.h"
#include "Buffer.h"
#include "InetAddress.h"

#include <memory>

namespace sznet
{

namespace net
{

// 事件分发
class Channel;
// 事件循环
class EventLoop;
// socket封装
class Socket;

// TCP连接(已经连接好，无论主动还是被动)，生命期模糊
class TcpConnection : NonCopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
	// 网络数据状态
	struct NetStatInfo
	{
		// 收到数据字节
		uint64_t m_recvBytes;
		// 发送数据字节
		uint64_t m_sendBytes;

		NetStatInfo():
			m_recvBytes(0),
			m_sendBytes(0)
		{
		}
	};

public:
	TcpConnection(EventLoop* loop, const string& name, const uint32_t id, sockets::sz_sock sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
	~TcpConnection();

	// 获取TCP连接所属的IO事件循环
	EventLoop* getLoop() const 
	{ 
		return m_loop; 
	}
	// 获取连接名称
	const string& name() const 
	{ 
		return m_name; 
	}
	// 获取连接ID
	const uint32_t id() const
	{
		return m_id;
	}
	// 获取连接的本地地址
	const InetAddress& localAddress() const 
	{ 
		return m_localAddr; 
	}
	// 获取连接的远端地址
	const InetAddress& peerAddress() const 
	{ 
		return m_peerAddr; 
	}
	// 是否连接
	bool connected() const 
	{ 
		return m_state == kConnected;
	}
	// 是否断开连接
	bool disconnected() const 
	{ 
		return m_state == kDisconnected; 
	}
	// 发送数据 或者 写入发送缓冲区
	void send(const void* message, int len);
	void send(const StringPiece& message);
	void send(Buffer* message);
	// 半关闭，关闭写端
	// NOT thread safe, no simultaneous calling
	void shutdown(); 
	// 关闭连接
	void forceClose();
	// 延迟关闭连接
	void forceCloseWithDelay(double seconds);
	// 是否开启Nagle算法，true表示关闭
	void setTcpNoDelay(bool on);
	// 注册可读事件
	void startRead();
	// 取消可读事件
	void stopRead();
	// 是否注册可读事件
	// NOT thread safe, may race with start/stopReadInLoop
	bool isReading() const 
	{ 
		return m_reading; 
	}; 
	// 外部连接建立/断开回调函数
	void setConnectionCallback(const ConnectionCallback& cb)
	{
		m_connectionCallback = cb;
	}
	// 外部处理收到消息的回调调函数
	void setMessageCallback(const MessageCallback& cb)
	{
		m_messageCallback = cb;
	}
	// 设置发送的数据全部发送完毕的回调函数
	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{
		m_writeCompleteCallback = cb;
	}
	// 设置高水位线，对应的高水位线回调函数
	void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
	{
		m_highWaterMarkCallback = cb; 
		m_highWaterMark = highWaterMark;
	}
	// 返回接收缓冲区
	Buffer* inputBuffer()
	{
		return &m_inputBuffer;
	}
	// 返回发送缓冲区
	Buffer* outputBuffer()
	{
		return &m_outputBuffer;
	}
	// 设置内部连接断开回调函数
	void setCloseCallback(const CloseCallback& cb)
	{
		m_closeCallback = cb;
	}
	// 连接建立，在TcpServer中建立连接后会调用此函数，注册读事件
	// called when TcpServer accepts a new connection
	// should be called only once
	void connectEstablished();
	// 关闭连接，提供给TcpServer使用，移除所有事件
	// called when TcpServer has removed me from its map
	// should be called only once
	void connectDestroyed();
	// 统计网络数据，竞态影响应该不大
	void calcNetData(NetStatInfo& deltaInfo, double& dTick)
	{
		auto tick = Timestamp::now();
		dTick = timeDifference(tick, m_calcNetLastTime);
		m_calcNetLastTime = tick;

		uint64_t deltaRecv = m_netStatInfo.m_recvBytes - m_prevNetStatInfo.m_recvBytes;
		m_prevNetStatInfo.m_recvBytes = m_netStatInfo.m_recvBytes;
		
		uint64_t deltaSend = m_netStatInfo.m_sendBytes - m_prevNetStatInfo.m_sendBytes;
		m_prevNetStatInfo.m_sendBytes = m_netStatInfo.m_sendBytes;

		deltaInfo.m_recvBytes = static_cast<uint64_t>(deltaRecv / dTick);
		deltaInfo.m_sendBytes = static_cast<uint64_t>(deltaSend / dTick);
	}
	// shrink收发缓冲区
	void shrinkRSBuffer(size_t sendSize, size_t recvSize);
	// m_loop所在线程中关闭连接，关闭写端
	void forceCloseInLoop();

private:
	// 连接状态
	enum StateE 
	{ 
		// 底层连接断开
		kDisconnected, 
		// 底层连接成功，listen所在的loop线程
		kConnecting, 
		// 逻辑层连接成功，所绑定loop线程回调connectEstablished设置
		kConnected, 
		// 连接正在断开连接，处于关闭写端
		kDisconnecting 
	};
	// socket可读事件
	void handleRead(Timestamp receiveTime);
	// socket可写事件
	void handleWrite();
	// socket关闭事件
	void handleClose();
	// socket错误事件
	void handleError();
	// m_loop所在线程中send
	void sendInLoop(const StringPiece& message);
	void sendInLoop(const void* message, size_t len);
	// m_loop所在线程中半关闭，关闭写端
	void shutdownInLoop();
	// 设置状态
	void setState(StateE s) 
	{ 
		m_state = s; 
	}
	// 把tcp连接状态转换成字符串形式
	const char* stateToString() const;
	//  m_loop所在线程中事件对象关注可读事件
	void startReadInLoop();
	// m_loop所在线程中事件对象取消可读事件
	void stopReadInLoop();
	// m_loop所在线程中shrink收发缓冲区
	void shrinkRSBufferInLoop(size_t sendSize, size_t recvSize);

	// 所属IO事件循环
	EventLoop* m_loop;
	// 连接名称
	const string m_name;
	// 连接ID
	uint32_t m_id;
	// 连接状态
	StateE m_state;
	// 是否注册可读事件
	bool m_reading;
	// 已经建立好的socket
	std::unique_ptr<Socket> m_socket;
	// 事件
	std::unique_ptr<Channel> m_channel;
	// 本地服务器地址
	const InetAddress m_localAddr;
	// 对方客户端地址
	const InetAddress m_peerAddr;
	// TCP连接建立/断开回调函数
	ConnectionCallback m_connectionCallback;
	// TCP处理收到消息的回调调函数
	MessageCallback m_messageCallback;
	// TCP数据发送完成的回调函数，直接发送成功/发送缓冲区发送完毕
	WriteCompleteCallback m_writeCompleteCallback;
	// TCP发送高水位线回调函数，上升沿触发一次
	HighWaterMarkCallback m_highWaterMarkCallback;
	// TCP连接断开回调函数
	CloseCallback m_closeCallback;
	// 发送高水位线
	size_t m_highWaterMark;
	// 接收缓冲区
	Buffer m_inputBuffer;
	// 发送缓冲区
	Buffer m_outputBuffer;
	// 当前网络数据
	NetStatInfo m_netStatInfo;
	// 上一次网络数据
	NetStatInfo m_prevNetStatInfo;
	// 上一次统计网络的时间
	Timestamp m_calcNetLastTime;
};

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_TCPCONNECTION_H_
