#include "SocketsOps.h"
#include "Endian.h"
#include "../process/Process.h"
#include "../log/Logging.h"

#if SZ_OS_LINUX
#	include <sys/eventfd.h>
#	include <fcntl.h>
#	include <arpa/inet.h>
#	include <netinet/tcp.h>
#	include <sys/uio.h>
#endif

#if defined(SZ_OS_WINDOWS)
#	include <ws2tcpip.h>
#endif

// 设置非阻塞 & fork exec 关闭
void setNonBlockAndCloseOnExec(sznet::net::sockets::sz_sock sockfd)
{
#if defined(SZ_OS_LINUX)
	// non-block
	int flags = ::fcntl(sockfd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	int ret = ::fcntl(sockfd, F_SETFL, flags);

	// close-on-exec
	flags = ::fcntl(sockfd, F_GETFD, 0);
	flags |= FD_CLOEXEC;
	ret = ::fcntl(sockfd, F_SETFD, flags);
	(void)ret;
#else
	u_long flag = 1;
	ioctlsocket(sockfd, FIONBIO, &flag);
#endif
}

namespace sznet
{

namespace net
{

namespace sockets
{

int sz_sock_setopt(sz_sock sock, int level, int option, int val)
{
	return setsockopt(sock, level, option, (const char*)&val, sizeof(val));
}

int sz_sock_getopt(sz_sock sock, int level, int option, int& val)
{
	sz_socklen_t len = static_cast<sz_socklen_t>(sizeof(val));
	return getsockopt(sock, level, option, (char*)&val, &len);
}

int sz_sock_setnonblock(sz_sock sock, bool nonblock)
{
#ifdef SZ_OS_LINUX
	int flag = ::fcntl(sock, F_GETFL, 0);
	if (flag == -1)
	{
		return -1;
	}

	if (nonblock)
	{
		flag |= O_NONBLOCK;
	}
	else
	{
		flag &= ~O_NONBLOCK;
	}
	return ::fcntl(sock, F_SETFL, flag);
#else
	u_long flag = nonblock ? 1 : 0;
	return ioctlsocket(sock, FIONBIO, &flag);
#endif
}

int sz_closesocket(sz_sock sock)
{
#ifdef SZ_OS_LINUX
	return ::close(sock);
#else
	return closesocket(sock);
#endif
}

int sz_close_eventfd(sz_event& event)
{
#ifdef SZ_OS_LINUX
	return ::close(event);
#else
	closesocket(event.event_read);
	return closesocket(event.event_write);
#endif
}

int sz_sock_settcpnodelay(sz_sock sock, bool on)
{
	// 1 表示禁用
	int flag = on ? 1 : 0;
	return sz_sock_setopt(sock, IPPROTO_TCP, TCP_NODELAY, flag);
}

int sz_sock_setreuseaddr(sz_sock sock, bool reuseaddr)
{
	int flag = reuseaddr ? 1 : 0;
	return sz_sock_setopt(sock, SOL_SOCKET, SO_REUSEADDR, flag);
}

int sz_sock_setkeepalive(sz_sock sock, bool on)
{
	int flag = on ? 1 : 0;
	return sz_sock_setopt(sock, SOL_SOCKET, SO_KEEPALIVE, flag);
}

bool sz_wouldblock()
{
	return sz_getlasterr() == sz_err_wouldblock || 
		sz_getlasterr() == sz_err_inprogress || 
		sz_getlasterr() == sz_err_again;
}

sz_ssize_t sz_readv(sz_sock sockfd, sz_iov_type* iov, int iovcnt)
{
#ifdef SZ_OS_WINDOWS
	DWORD bytesRead;
	DWORD flags = 0;
	if (WSARecv(sockfd, iov, iovcnt, &bytesRead, &flags, nullptr, nullptr) != 0)
	{
		return -1;
	}
	return bytesRead;
#else
	return ::readv(sockfd, iov, iovcnt);
#endif
}

const struct sockaddr* sz_sockaddr_cast(const struct sockaddr_in* addr)
{
	return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}

struct sockaddr* sz_sockaddr_cast(struct sockaddr_in* addr)
{
	return static_cast<struct sockaddr*>(implicit_cast<void*>(addr));
}

const struct sockaddr* sz_sockaddr_cast(const struct sockaddr_in6* addr)
{
	return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}

struct sockaddr* sz_sockaddr_cast(struct sockaddr_in6* addr)
{
	return static_cast<struct sockaddr*>(implicit_cast<void*>(addr));
}

const struct sockaddr_in* sz_sockaddrin_cast(const struct sockaddr* addr)
{
	return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(addr));
}

struct sockaddr_in* sz_sockaddrin_cast(struct sockaddr* addr)
{
	return static_cast<struct sockaddr_in*>(implicit_cast<void*>(addr));
}

const struct sockaddr_in6* sz_sockaddrin6_cast(const struct sockaddr* addr)
{
	return static_cast<const struct sockaddr_in6*>(implicit_cast<const void*>(addr));
}

struct sockaddr_in6* sz_sockaddrin6_cast(struct sockaddr* addr)
{
	return static_cast<struct sockaddr_in6*>(implicit_cast<void*>(addr));
}

void sz_fromipport(const char* ip, uint16_t port, struct sockaddr_in* addr)
{
	addr->sin_family = AF_INET;
	addr->sin_port = hostToNetwork16(port);
	if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
	{
		LOG_SYSERR << "sockets::sz_fromipport";
	}
}

void sz_fromipport(const char* ip, uint16_t port, struct sockaddr_in6* addr)
{
	addr->sin6_family = AF_INET6;
	addr->sin6_port = hostToNetwork16(port);
	if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0)
	{
		LOG_SYSERR << "sockets::sz_fromipport";
	}
}

void sz_toip(char* buf, size_t size, const struct sockaddr* addr)
{
	if (addr->sa_family == AF_INET)
	{
		assert(size >= INET_ADDRSTRLEN);
		const struct sockaddr_in* addr4 = sz_sockaddrin_cast(addr);
		::inet_ntop(AF_INET, (void*)(&addr4->sin_addr), buf, size);
	}
	else if (addr->sa_family == AF_INET6)
	{
		assert(size >= INET6_ADDRSTRLEN);
		const struct sockaddr_in6* addr6 = sz_sockaddrin6_cast(addr);
		::inet_ntop(AF_INET6, (void*)(&addr6->sin6_addr), buf, size);
	}
}

void sz_toipport(char* buf, size_t size, const struct sockaddr* addr)
{
	if (addr->sa_family == AF_INET6)
	{
		buf[0] = '[';
		sz_toip(buf + 1, size - 1, addr);
		size_t end = ::strlen(buf);
		const struct sockaddr_in6* addr6 = sz_sockaddrin6_cast(addr);
		uint16_t port = sockets::networkToHost16(addr6->sin6_port);
		assert(size > end);
		snprintf(buf + end, size - end, "]:%u", port);
		return;
	}
	sz_toip(buf, size, addr);
	size_t end = ::strlen(buf);
	const struct sockaddr_in* addr4 = sz_sockaddrin_cast(addr);
	uint16_t port = sockets::networkToHost16(addr4->sin_port);
	assert(size > end);
	snprintf(buf + end, size - end, ":%u", port);
}

void sz_bindordie(sz_sock sockfd, const struct sockaddr* addr)
{
	int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr)));
	if (ret < 0)
	{
		LOG_SYSFATAL << "sockets::sz_bindordie";
	}
}

void sz_listenordie(sz_sock sockfd)
{
	int ret = ::listen(sockfd, SOMAXCONN);
	if (ret < 0)
	{
		LOG_SYSFATAL << "sockets::sz_listenordie";
	}
}

sz_sock sz_accept(sz_sock sockfd, struct sockaddr_in6* addr)
{
	socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
	sz_sock connfd = ::accept(sockfd, sz_sockaddr_cast(addr), &addrlen);
	if (connfd < 0)
	{
		LOG_FATAL << "sockets::sz_accept";
	}
	else
	{
		setNonBlockAndCloseOnExec(connfd);
	}

	return connfd;
}

void sz_shutdownwrite(sz_sock sockfd)
{
#ifdef SZ_OS_WINDOWS
	if (::shutdown(sockfd, SD_SEND) < 0)
#else
	if (::shutdown(sockfd, SHUT_WR) < 0)
#endif
	{
		LOG_SYSERR << "sockets::sz_shutdownwrite";
	}
}

sz_ssize_t sz_socket_write(sz_sock sockfd, const void* buf, size_t count)
{
#ifdef SZ_OS_WINDOWS
	static int maxInt32 = std::numeric_limits<int>::max(); (void)maxInt32;
	if (unlikely(count > maxInt32))
	{
		count = maxInt32;
	}
	return ::send(sockfd, static_cast<const char*>(buf), static_cast<int>(count), 0);
#else
	return ::send(sockfd, static_cast<const char*>(buf), count, 0);
#endif
}

int sz_sock_geterror(sz_sock sockfd)
{
	int optval;
	socklen_t optlen = static_cast<socklen_t>(sizeof(optval));
	if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&optval, &optlen) < 0)
	{
		return sz_getlasterr();
	}
	else
	{
		return optval;
	}
}

sz_sock sz_sock_createnonblockingordie(int family)
{
	sz_sock sockfd = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0)
	{
		LOG_SYSFATAL << "sockets::sz_sock_createnonblockingordie";
	}
	setNonBlockAndCloseOnExec(sockfd);

	return sockfd;
}

int sz_sock_connect(sz_sock sockfd, const struct sockaddr* addr)
{
	return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr)));
}

struct sockaddr_in6 sz_sock_getlocaladdr(sz_sock sockfd)
{
	struct sockaddr_in6 localaddr;
	memset(&localaddr, 0, sizeof(localaddr));
	socklen_t addrlen = static_cast<socklen_t>(sizeof(localaddr));
	if (::getsockname(sockfd, sz_sockaddr_cast(&localaddr), &addrlen) < 0)
	{
		LOG_SYSERR << "sockets::sz_sock_getlocaladdr";
	}
	return localaddr;
}

struct sockaddr_in6 sz_sock_getpeeraddr(sz_sock sockfd)
{
	struct sockaddr_in6 peeraddr;
	memset(&peeraddr, 0, sizeof(peeraddr));
	socklen_t addrlen = static_cast<socklen_t>(sizeof(peeraddr));
	if (::getpeername(sockfd, sz_sockaddr_cast(&peeraddr), &addrlen) < 0)
	{
		LOG_SYSERR << "sockets::sz_sock_getpeeraddr";
	}
	return peeraddr;
}

bool sz_sock_isselfconnect(sz_sock sockfd)
{
	struct sockaddr_in6 localaddr = sz_sock_getlocaladdr(sockfd);
	struct sockaddr_in6 peeraddr = sz_sock_getpeeraddr(sockfd);
	if (localaddr.sin6_family == AF_INET)
	{
		const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
		const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
		return laddr4->sin_port == raddr4->sin_port
			&& laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
	}
	else if (localaddr.sin6_family == AF_INET6)
	{
		return localaddr.sin6_port == peeraddr.sin6_port
			&& memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof(localaddr.sin6_addr)) == 0;
	}
	else
	{
		return false;
	}
}

#if defined(SZ_OS_WINDOWS)

sz_event sz_create_eventfd()
{
	sz_event sock_event;
	sock_event.event_read = sz_invalid_socket;
	sock_event.event_write = sz_invalid_socket;
	sockaddr_in name;
	memset(&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	name.sin_port = 0;
	int namelen = sizeof(name);
	int rst = 0;

	sz_sock tcp = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp == sz_invalid_socket)
	{
		rst = -1;
		goto clean;
	}
	if (bind(tcp, (sockaddr*)&name, namelen) == sz_invalid_socket) 
	{
		rst = -2;
		goto clean;
	}
	if (listen(tcp, 5) == sz_invalid_socket) 
	{
		rst = -3;
		goto clean;
	}
	if (getsockname(tcp, (sockaddr*)&name, &namelen) == sz_invalid_socket)
	{
		rst = -4;
		goto clean;
	}
	sock_event.event_read = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_event.event_read == sz_invalid_socket)
	{
		rst = -5;
		goto clean;
	}
	if (connect(sock_event.event_read, (sockaddr*)&name, namelen) == sz_invalid_socket) 
	{
		rst = -6;
		goto clean;
	}
	if (sz_sock_setnonblock(sock_event.event_read, true) != 0)
	{
		rst = -7;
		goto clean;
	}
	if (sz_sock_settcpnodelay(sock_event.event_read, true) != 0)
	{
		rst = -8;
		goto clean;
	}
	sock_event.event_write = accept(tcp, (sockaddr*)&name, &namelen);
	if (sock_event.event_write == sz_invalid_socket) 
	{
		rst = -9;
		goto clean;
	}
	if (sz_sock_setnonblock(sock_event.event_write, true) != 0)
	{
		rst = -10;
		goto clean;
	}
	if (sz_sock_settcpnodelay(sock_event.event_write, true) != 0)
	{
		rst = -11;
		goto clean;
	}
	if (sz_closesocket(tcp) == sz_invalid_socket)
	{
		rst = -12;
		goto clean;
	}

	return sock_event;

clean:
	LOG_SYSERR << "Failed in eventfd and rst = " << rst;
	if (tcp != sz_invalid_socket) 
	{
		sz_closesocket(tcp);
	}
	if (sock_event.event_read != sz_invalid_socket)
	{
		sz_closesocket(sock_event.event_read);
	}
	if (sock_event.event_write != sz_invalid_socket)
	{
		sz_closesocket(sock_event.event_write);
	}
	abort();

	// return sock_event;
}

#else

sz_event sz_create_eventfd()
{
	int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (evtfd < 0)
	{
		LOG_SYSERR << "Failed in eventfd";
		abort();
	}

	return evtfd;
}

#endif


} // end namespace sockets

} // end namespace net

} // end namespace sznet
