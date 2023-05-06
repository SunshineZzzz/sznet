#include "Socket.h"
#include "../log/Logging.h"
#include "InetAddress.h"

#ifdef SZ_OS_LINUX
	#include <netinet/in.h>
	#include <netinet/tcp.h>
#endif

#include <stdio.h>

namespace sznet
{

namespace net
{

Socket::~Socket()
{
	sockets::sz_closesocket(m_sockfd);
}

void Socket::bindAddress(const InetAddress& addr)
{
	sockets::sz_bindordie(m_sockfd, addr.getSockAddr());
}

void Socket::listen()
{
	sockets::sz_listenordie(m_sockfd);
}

sockets::sz_sock Socket::accept(InetAddress* peeraddr)
{
	struct sockaddr_in6 addr;
	memset(&addr, 0, sizeof(addr));
	sockets::sz_sock connfd = sockets::sz_accept(m_sockfd, &addr);
	if (connfd >= 0)
	{
		peeraddr->setSockAddrInet6(addr);
	}
	return connfd;
}

void Socket::shutdownWrite()
{
	sockets::sz_shutdownwrite(m_sockfd);
}

void Socket::setTcpNoDelay(bool on)
{
	sockets::sz_sock_settcpnodelay(m_sockfd, on);
}

void Socket::setReuseAddr(bool on)
{
	sockets::sz_sock_setreuseaddr(m_sockfd, on);
}

void Socket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT
	int optval = on ? 1 : 0;
	int ret = ::setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));
	if (ret < 0 && on)
	{
		LOG_SYSERR << "SO_REUSEPORT failed.";
	}
#else
	if (on)
	{
		LOG_ERROR << "SO_REUSEPORT is not supported.";
	}
#endif
}

void Socket::setKeepAlive(bool on)
{
	sockets::sz_sock_setkeepalive(m_sockfd, on);
}

} // end namespace net

} // end namespace sznet