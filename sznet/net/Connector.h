// Comment: 主动连接实现

#ifndef _SZNET_NET_CONNECTOR_H_
#define _SZNET_NET_CONNECTOR_H_

#include "../base/NonCopyable.h" 
#include "InetAddress.h"

#include <functional>
#include <memory>

namespace sznet
{

namespace net
{

// 事件分发
class Channel;
// IO事件循环
class EventLoop;

// 非阻塞主动发起发起连接，带有重连功能
class Connector : NonCopyable, public std::enable_shared_from_this<Connector>
{
public:
	// 连接成功的回调函数类型
	typedef std::function<void(sockets::sz_sock sockfd)> NewConnectionCallback;

	Connector(EventLoop* loop, const InetAddress& serverAddr);
	~Connector();

	// 设置连接成功的回调函数
	void setNewConnectionCallback(const NewConnectionCallback& cb)
	{
		m_newConnectionCallback = cb;
	}
	// 发起连接
	// can be called in any thread
	void start();
	// 重新发起连接
	// must be called in loop thread
	void restart();
	// 停止重连
	// can be called in any thread
	void stop();
	// 返回server的地址
	const InetAddress& serverAddress() const 
	{ 
		return m_serverAddr; 
	}

private:
	// 状态
	enum States 
	{ 
		// 断开连接
		kDisconnected, 
		// 正在连接
		kConnecting, 
		// 已经连接
		kConnected 
	};
	// 最大重连延迟
	static const int kMaxRetryDelayMs = 30 * 1000;
	// 初次重连延迟
	static const int kInitRetryDelayMs = 500;

	// 设置连接状态
	void setState(States s) 
	{ 
		m_state = s;
	}
	// m_loop所在线程中建立连接
	void startInLoop();
	// 停止当前的连接尝试，重新发起来连接
	void stopInLoop();
	// 发起连接
	void connect();
	// 正在连接
	void connecting(sockets::sz_sock sockfd);
	// 非阻塞connect可写，但是不一定表示已经建立连接 ，需要获取err来判断
	void handleWrite();
	// 出错
	void handleError();
	// 尝试重新连接
	void retry(sockets::sz_sock sockfd);
	// 清除事件 && 移除m_channel
	sockets::sz_sock removeAndResetChannel();
	// reset后m_channel为空
	void resetChannel();

	// 所属的EventLoop
	EventLoop* m_loop;
	// server地址
	InetAddress m_serverAddr;
	// 是否允许发起重连
	// atomic
	bool m_connect;
	// 连接状态
	// FIXME: use atomic variable
	States m_state;
	// 事件分发
	std::unique_ptr<Channel> m_channel;
	// 连接成功后的回调函数
	NewConnectionCallback m_newConnectionCallback;
	// 重连延迟
	int m_retryDelayMs;
};

} // namespace net

} // namespace sznet

#endif // _SZNET_NET_CONNECTOR_H_
