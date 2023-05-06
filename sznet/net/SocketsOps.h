// Comment:

#ifndef _SZNET_NET_SOCKETSOPS_H_
#define _SZNET_NET_SOCKETSOPS_H_

#include "../NetCmn.h"

#if SZ_OS_WINDOWS
#	include <WinSock2.h>
#	include <Ws2ipdef.h>
#endif

#if SZ_OS_LINUX
#	include <sys/socket.h>
#	include <netinet/in.h>
#endif

#include <limits>

namespace sznet
{

namespace net
{

namespace sockets
{

#if SZ_COMPILER_MSVC
#	define sz_invalid_socket INVALID_SOCKET
	using sz_fd = SOCKET;
	using sz_sock = SOCKET;
	using sz_socklen_t = int;
	struct sz_event
	{
		SOCKET event_read = sz_invalid_socket;
		SOCKET event_write = sz_invalid_socket;
	};
#	define sz_err_wouldblock WSAEWOULDBLOCK
#	define sz_err_inprogress WSAEINPROGRESS
#	define sz_err_again WSAEWOULDBLOCK
#	define sz_err_eintr WSAEINTR
#	define sz_err_emfile WSAEMFILE
#	define sz_err_eisconn WSAEISCONN
#	define sz_err_eaddrinuse WSAEADDRINUSE 
#	define sz_err_eaddrnotavail WSAEADDRNOTAVAIL
#	define sz_err_econnrefused WSAECONNREFUSED
#	define sz_err_enetunreach WSAENETUNREACH
#	define sz_err_eacces WSAEACCES
#	define sz_err_eperm WSAEACCES
#	define sz_err_eafnosupport WSAEAFNOSUPPORT
#	define sz_err_ealready WSAEALREADY
#	define sz_err_ebadf WSAEBADF
#	define sz_err_efault WSAEFAULT
#	define sz_err_enotsock WSAENOTSOCK
	using sz_iov_type = WSABUF;
#   define sz_set_iov_buf(iov, ptr) iov.buf=ptr
#	define sz_set_iov_buflen(iov, size) \
		if(unlikely(size > std::numeric_limits<ULONG>::max())) \
		{ \
			assert(0);\
		} \
		iov.len=static_cast<ULONG>(size);
#endif

#if SZ_COMPILER_GNUC 
#	define sz_invalid_socket -1
	using sz_fd = int;
	using sz_sock = int;
	using sz_socklen_t = socklen_t;
	using sz_event = int;
	// send
	// When the message does not fit into the send buffer of the socket, send() normally blocks, 
	// unless the socket has been placed in nonblocking I/O mode. 
	// In nonblocking mode it would fail with the error EAGAIN or EWOULDBLOCK in this case.
	// The select(2) call may be used to determine when it is possible to send more data.
	// recv
	// If no messages are available at the socket, the receive calls wait for a message to arrive,  
	// unless the socket is nonblocking(see fcntl(2)), in which case the value - 1 is returned 
	// and the external variable errno is set to EAGAIN or EWOULDBLOCK.  
#	define sz_err_wouldblock EWOULDBLOCK
#	define sz_err_again EAGAIN
	// connect
	// The socket is nonblocking and the connection cannot be completed immediately.
	// It is possible to select(2) or poll(2) for completion by selecting the socket for writing.
	// After select(2) indicates writability, use getsockopt(2) to read the SO_ERROR option 
	// at level SOL_SOCKET to determine whether connect() completed successfully(SO_ERROR is zero) 
	// or unsuccessfully(SO_ERROR is one of the usual error codes listed here, explaining the reason for the failure).
#	define sz_err_inprogress EINPROGRESS
	// Interrupted function call
#	define sz_err_eintr EINTR
	// Too many open files
#	define sz_err_emfile EMFILE
	// The socket is already connected.
#	define sz_err_eisconn EISCONN
	// Local address is already in use.
#	define sz_err_eaddrinuse EADDRINUSE
	// Non-existent interface was requested or the requested address was not local.
#	define sz_err_eaddrnotavail EADDRNOTAVAIL
	// No-one listening on the remote address.
#	define sz_err_econnrefused ECONNREFUSED
	// Network is unreachable.
#	define sz_err_enetunreach ENETUNREACH
	// The user tried to connect to a broadcast address without having the 
	// socket broadcast flag enabled or the connection request failed because of a local firewall rule.
#	define sz_err_eacces EACCES
#	define sz_err_eperm EPERM
	// The passed address didn't have the correct address family in its sa_family field.
#	define sz_err_eafnosupport EAFNOSUPPORT
	// The socket is nonblocking and a previous connection attempt has not yet been completed.
#	define sz_err_ealready EALREADY
	// The file descriptor is not a valid index in the descriptor table.
#	define sz_err_ebadf EBADF
	// The socket structure address is outside the user's address space.
#	define sz_err_efault EFAULT
	// The file descriptor is not associated with a socket
#	define sz_err_enotsock ENOTSOCK
	using sz_iov_type = struct iovec;
#   define sz_set_iov_buf(iov, ptr) iov.iov_base=ptr
#	define sz_set_iov_buflen(iov, size) iov.iov_len=size
#endif

// socket属性
int sz_sock_setopt(sz_sock sock, int level, int option, int val);
int sz_sock_getopt(sz_sock sock, int level, int option, int& val);
// 设置非阻塞或者非阻塞
int sz_sock_setnonblock(sz_sock sock, bool nonblock);
// 销毁socket
int sz_closesocket(sz_sock sock);
// 销毁eventfd
int sz_close_eventfd(sz_event& event);
// 禁用/开启 Nagle Algorithm
int sz_sock_settcpnodelay(sz_sock sock, bool on);
// 地址重用
int sz_sock_setreuseaddr(sz_sock sock, bool reuseaddr);
// 是否开启保活
int sz_sock_setkeepalive(sz_sock sock, bool on);
// 创建eventfd
sz_event sz_create_eventfd();
// 上一次错误是否为阻塞
bool sz_wouldblock();
// 从socket中读取数据到多个缓冲区
sz_ssize_t sz_readv(sz_sock sockfd, sz_iov_type* iov, int iovcnt);
// 从ip和port转化到sockaddr_in类型
void sz_fromipport(const char* ip, uint16_t port, struct sockaddr_in* addr);
// 从ip和port转化到sockaddr_in6类型
void sz_fromipport(const char* ip, uint16_t port, struct sockaddr_in6* addr);
// sockaddr_in转sockaddr
const struct sockaddr* sz_sockaddr_cast(const struct sockaddr_in* addr);
struct sockaddr* sz_sockaddr_cast(struct sockaddr_in* addr);
// sockaddr_in6转sockaddr
const struct sockaddr* sz_sockaddr_cast(const struct sockaddr_in6* addr);
struct sockaddr* sz_sockaddr_cast(struct sockaddr_in6* addr);
// sockaddr转sockaddr_in
const struct sockaddr_in* sz_sockaddrin_cast(const struct sockaddr* addr);
struct sockaddr_in* sz_sockaddrin_cast(struct sockaddr* addr);
// sockaddr转sockaddr_in6
const struct sockaddr_in6* sz_sockaddrin6_cast(const struct sockaddr* addr);
struct sockaddr_in6* sz_sockaddrin6_cast(struct sockaddr* addr);
// 提取出sockaddr的ip到点分十进制
void sz_toip(char* buf, size_t size, const struct sockaddr* addr);
// 提取出sockaddr的ip和port
void sz_toipport(char* buf, size_t size, const struct sockaddr* addr);
// 绑定
void sz_bindordie(sz_sock sockfd, const struct sockaddr* addr);
// 监听
void sz_listenordie(sz_sock sockfd);
// 接收新连接并且设置为非阻塞
sz_sock sz_accept(sz_sock sockfd, struct sockaddr_in6* addr);
// socket关闭write流
void sz_shutdownwrite(sz_sock sockfd);
// socket发送消息
sz_ssize_t sz_socket_write(sz_sock sockfd, const void* buf, size_t count);
// 获取socket错误
int sz_sock_geterror(sz_sock sockfd);
// 创建非阻塞socket
sz_sock sz_sock_createnonblockingordie(int family);
// 发起连接
int sz_sock_connect(sz_sock sockfd, const struct sockaddr* addr);
// 获取socket本地的地址信息
struct sockaddr_in6 sz_sock_getlocaladdr(sz_sock sockfd);
// 获取socket对端的地址信息
struct sockaddr_in6 sz_sock_getpeeraddr(sz_sock sockfd);
// 判断是否发生自连接
bool sz_sock_isselfconnect(sz_sock sockfd);

} // end namespace sockets

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_SOCKETSOPS_H_
