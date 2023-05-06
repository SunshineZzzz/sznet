// Comment: 连接接收器实现

#ifndef _SZNET_NET_ACCEPTOR_H_
#define _SZNET_NET_ACCEPTOR_H_

#include "Socket.h"
#include "Channel.h"

#include <functional>

namespace sznet
{

namespace net
{

// IO事件循环
class EventLoop;
// 网络地址
class InetAddress;

// 连接接收器
class Acceptor : NonCopyable
{
public:
	// 新连接回调函数类型
	typedef std::function<void(sockets::sz_sock sockfd, const InetAddress&)> NewConnectionCallback;

	Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
	~Acceptor();

	// 设置新连接的回调函数
	void setNewConnectionCallback(const NewConnectionCallback& cb)
	{
		m_newConnectionCallback = cb;
	}
	// 是否处于监听
	bool listenning() const 
	{ 
		return m_listenning; 
	}
	// 启动监听
	void listen();

private:
	// 可读事件，接受新连接
	void handleRead();

	// 所属的IO事件循环
	EventLoop* m_loop;
	// 监听socket
	Socket m_acceptSocket;
	// 对应的事件分发
	Channel m_acceptChannel;
	// 用户定义的新连接回调函数
	NewConnectionCallback m_newConnectionCallback;
	// 是否正在监听状态
	bool m_listenning;
	// m_idleFd作用防止busy loop：
	// 如果此时你的最大连接数达到了上限，而accept队列里可能还一直在增加新的连接等你接受，
	// sznet用的是epoll的LT模式时，那么如果因为你连接达到了文件描述符的上限，
	// 此时没有可供你保存新连接套接字描述符的文件符了，
	// 那么新来的连接就会一直放在accept队列中，于是呼其对应的可读事件就会
	// 一直触发读事件(因为你一直不读，也没办法读走它)，此时就会造成我们常说的busy loop
	sockets::sz_sock m_idleFd;
};

} // namespace net

} // namespace sznet

#endif // _SZNET_NET_ACCEPTOR_H_
