#ifndef _SZNET_NET_SOCKETSOPS_H_
#define _SZNET_NET_SOCKETSOPS_H_

#include "../NetCmn.h"

#if SZ_OS_WINDOWS
#	include <WinSock2.h>
#endif

#if SZ_OS_LINUX
#endif

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
#	define sz_err_eintr EINTR
#endif

// ���÷��������߷�����
int sz_sock_setnonblock(sz_sock sock, bool nonblock);
// ����socket
int sz_closesocket(sz_sock sock);
// ����eventfd
sz_event sz_create_eventfd();
// ����eventfd
int sz_close_eventfd(sz_event& event);

} // end namespace sockets

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_SOCKETSOPS_H_
