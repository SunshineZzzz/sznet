// Comment: socket实现

#ifndef _SZNET_NET_SOCKET_H_
#define _SZNET_NET_SOCKET_H_

#include "../NetCmn.h"
#include "../base/NonCopyable.h"
#include "SocketsOps.h"

namespace sznet
{

namespace net
{

// 网络地址
class InetAddress;

// socket描述符fd的封装
class Socket : NonCopyable
{
public:
	explicit Socket(sockets::sz_sock sockfd): 
		m_sockfd(sockfd)
	{
	}
	~Socket();

	// 返回fd
	sockets::sz_sock fd() const
	{ 
		return m_sockfd;
	}
	// 绑定网络地址
	void bindAddress(const InetAddress& localaddr);
	// 监听
	void listen();
	// 接收新连接，成功返回non-blocking和close-on-exec属性的connfd
	sockets::sz_sock accept(InetAddress* peeraddr);
	// 关闭write流
	void shutdownWrite();
	// 是否使用 Nagle 算法，低延迟程序关闭
	void setTcpNoDelay(bool on);
	// 是否SO_REUSEADDR
	void setReuseAddr(bool on);
	// 是否SO_REUSEPORT
	void setReusePort(bool on);
	// 是否开启保活，断电了本机TCP收不到FIN，最好编写逻辑层心跳
	void setKeepAlive(bool on);

private:
	// socket fd
	const sockets::sz_sock m_sockfd;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_SOCKET_H_
