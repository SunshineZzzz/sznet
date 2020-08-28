#include "SocketsOps.h"
#include "../log/Logging.h"

#if SZ_OS_LINUX
#	include <sys/eventfd.h>
#endif

namespace
{
// 初始化网络环境
class InitSocket
{
public:
	InitSocket()
	{
#if defined (SZ_OS_WINDOWS)
		WSADATA  Ws;
		if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0)
		{
			LOG_SYSFATAL << "WSAStartup failed!";
		}
		else
		{
			LOG_INFO << "WSAStartup success!";
		}
#endif
	}
};
InitSocket initWSA;

} // end namespace

namespace sznet
{

namespace net
{

namespace sockets
{

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
#endif
#ifdef SZ_OS_WINDOWS
	u_long flag = nonblock ? 1 : 0;
	return ioctlsocket(sock, FIONBIO, &flag);
#endif
}

int sz_closesocket(sz_sock sock)
{
#ifdef SZ_OS_LINUX
	return ::close(sock);
#endif
#ifdef SZ_OS_WINDOWS
	return closesocket(sock);
#endif
}

int sz_close_eventfd(sz_event& event)
{
#ifdef SZ_OS_LINUX
	return ::close(event);
#endif
#ifdef SZ_OS_WINDOWS
	closesocket(event.event_read);
	return closesocket(event.event_write);
#endif
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

	sz_sock tcp = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp == sz_invalid_socket)
	{
		goto clean;
	}
	if (bind(tcp, (sockaddr*)&name, namelen) == sz_invalid_socket) 
	{
		goto clean;
	}
	if (listen(tcp, 5) == sz_invalid_socket) 
	{
		goto clean;
	}
	if (getsockname(tcp, (sockaddr*)&name, &namelen) == sz_invalid_socket)
	{
		goto clean;
	}
	sock_event.event_read = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_event.event_read == sz_invalid_socket)
	{
		goto clean;
	}
	if (connect(sock_event.event_read, (sockaddr*)&name, namelen) == sz_invalid_socket) 
	{
		goto clean;
	}
	sock_event.event_write = accept(tcp, (sockaddr*)&name, &namelen);
	if (sock_event.event_write == sz_invalid_socket) 
	{
		goto clean;
	}
	if (sz_closesocket(tcp) == sz_invalid_socket)
	{
		goto clean;
	}

	return sock_event;

clean:
	LOG_SYSERR << "Failed in eventfd";
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

	return sock_event;
}

#elif defined(SZ_OS_LINUX)

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

} // end namespace muduo
