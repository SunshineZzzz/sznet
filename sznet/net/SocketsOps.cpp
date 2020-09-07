#include "SocketsOps.h"
#include "../process/Process.h"
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
		WSADATA WS;
		if (WSAStartup(MAKEWORD(2, 2), &WS) != 0)
		{
			LOG_SYSFATAL << "WSAStartup failed!";
		}
		else
		{
			LOG_INFO << "WSAStartup success!";
		}
#endif
	}
	~InitSocket()
	{
#if defined (SZ_OS_WINDOWS)
		WSACleanup();
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

int sz_sock_setopt(sz_sock sock, int level, int option, int val)
{
	return setsockopt(sock, level, option, (const char*)&val, sizeof(val));
}

int sz_sock_getopt(sz_sock sock, int level, int option, int& val)
{
	int len = sizeof(val);
	return getsockopt(sock, level, option, (char*)&val, (sz_socklen_t*)&len);
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

int sz_sock_settcpnodelay(sz_sock sock)
{
	int flag = 1;
	return sz_sock_setopt(sock, IPPROTO_TCP, TCP_NODELAY, flag);
}

bool sz_wouldblock()
{
	return sz_getlasterr() == sz_err_wouldblock || 
		sz_getlasterr() == sz_err_inprogress || 
		sz_getlasterr() == sz_err_again;
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
	if (sz_sock_settcpnodelay(sock_event.event_read) != 0)
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
	if (sz_sock_settcpnodelay(sock_event.event_write) != 0)
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
