#include "InetAddress.h"
#include "Endian.h"
#include "SocketsOps.h"
#include "../log/Logging.h"

#include <thread>

#include <stddef.h>

#ifdef SZ_OS_WINDOWS
#	include <ws2tcpip.h>
#endif

#ifdef SZ_OS_LINUX
#	include <netinet/in.h>       
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <netdb.h>
#endif

namespace sznet 
{
	
namespace net
{

static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6), "InetAddress is same size as sockaddr_in6");
static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");

InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6)
{
	static_assert(offsetof(InetAddress, m_addr6) == 0, "m_addr6 offset 0");
	static_assert(offsetof(InetAddress, m_addr) == 0, "m_addr offset 0");
	if (ipv6)
	{
		memset(&m_addr6, 0, sizeof(m_addr6));
		m_addr6.sin6_family = AF_INET6;
		in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
		m_addr6.sin6_addr = ip;
		m_addr6.sin6_port = sockets::hostToNetwork16(port);
	}
	else
	{
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sin_family = AF_INET;
		auto ip = loopbackOnly ? INADDR_LOOPBACK : INADDR_ANY;
		m_addr.sin_addr.s_addr = sockets::hostToNetwork32(ip);
		m_addr.sin_port = sockets::hostToNetwork16(port);
	}
}

InetAddress::InetAddress(StringArg ip, uint16_t port, bool ipv6)
{
	if (ipv6 || strchr(ip.c_str(), ':'))
	{
		memset(&m_addr6, 0, sizeof(m_addr6));
		sockets::sz_fromipport(ip.c_str(), port, &m_addr6);
	}
	else
	{
		memset(&m_addr, 0, sizeof(m_addr));
		sockets::sz_fromipport(ip.c_str(), port, &m_addr);
	}
}

string InetAddress::toIp() const
{
	char buf[64] = "";
	sockets::sz_toip(buf, sizeof(buf), getSockAddr());
	return buf;
}

string InetAddress::toIpPort() const
{
	char buf[64] = "";
	sockets::sz_toipport(buf, sizeof(buf), getSockAddr());
	return buf;
}

uint16_t InetAddress::port() const
{
	return sockets::networkToHost16(portNetEndian());
}

uint32_t InetAddress::ipNetEndian() const
{
	assert(family() == AF_INET);
	return m_addr.sin_addr.s_addr;
}

#if false

#if defined(SZ_OS_LINUX)
// gethostbyname_r函数处理的缓冲区
static thread_local char t_resolveBuffer[64 * 1024];
#endif
bool InetAddress::resolve(StringArg hostname, InetAddress* out)
{
	assert(out != nullptr);
	struct hostent* he = nullptr;
	int ret = 0;
#if defined(SZ_OS_LINUX)
	struct hostent hent;
	int herrno = 0;
	memset(&hent, 0, sizeof(hent));
	ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof(t_resolveBuffer), &he, &herrno);
#else
	// The memory for the hostent structure returned by the gethostbyname 
	// function is allocated internally by the Winsock DLL from thread local storage.
	// 线程安全
	he = gethostbyname(hostname.c_str());
#endif
	if (ret == 0 && he != nullptr)
	{
		assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
		out->m_addr.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
		return true;
	}
	else
	{
		if (ret)
		{
			LOG_SYSERR << "InetAddress::resolve";
		}
		return false;
	}
}

#else

bool InetAddress::resolve(StringArg hostname, InetAddress* out)
{
	assert(out != nullptr);
	struct addrinfo *res = nullptr;
	int ret = getaddrinfo(hostname.c_str(), nullptr, 0, &res);
	if (ret == 0 && res != nullptr)
	{
		out->m_addr.sin_addr = *reinterpret_cast<struct in_addr*>(res->ai_addr);
		freeaddrinfo(res);
		return true;
	}
	else
	{
		if (ret)
		{
			LOG_SYSERR << "InetAddress::resolve";
		}
		return false;
	}
}

#endif

void InetAddress::setScopeId(uint32_t scope_id)
{
	if (family() == AF_INET6)
	{
		m_addr6.sin6_scope_id = scope_id;
	}
}

void InetAddress::resetPort(uint16_t port, bool ipv6)
{
	if (ipv6)
	{
		m_addr6.sin6_port = sockets::hostToNetwork16(port);
	}
	else
	{
		m_addr.sin_port = sockets::hostToNetwork16(port);
	}
}

} // end namespace net

} // end namespace sznet