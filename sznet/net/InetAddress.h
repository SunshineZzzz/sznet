// Comment: 网络地址实现

#ifndef _SZNET_NET_INETADDRESS_H_
#define _SZNET_NET_INETADDRESS_H_

#include "../NetCmn.h"
#include "../base/Copyable.h"
#include "../string/StringPiece.h"
#include "../net/SocketsOps.h"

#ifdef SZ_OS_LINUX
#	include <netinet/in.h>
#endif

#ifdef SZ_OS_WINDOWS
#	include <Ws2ipdef.h>
#endif

namespace sznet
{

namespace net
{

// 网络地址类，对sockaddr_in的一个包装，实现了各种针对sockaddr_in的操作
class InetAddress : public Copyable
{
public:
	explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);
	InetAddress(StringArg ip, uint16_t port, bool ipv6 = false);
	explicit InetAddress(const struct sockaddr_in& addr): 
		m_addr(addr)
	{
	}
	explicit InetAddress(const struct sockaddr_in6& addr): 
		m_addr6(addr)
	{ 
	}

	// 返回协议族
	unsigned short family() const 
	{ 
		return m_addr.sin_family; 
	}
	// 返回ip的字符串
	string toIp() const;
	// 返回ip和端口的字符串
	string toIpPort() const;
	// 返回端口
	uint16_t port() const;
	// 获取sockaddr
	const struct sockaddr* getSockAddr() const 
	{ 
		return sockets::sz_sockaddr_cast(&m_addr6); 
	}
	// 获取sockaddr_in
	const struct sockaddr_in* getSockAddrIn() const
	{
		return &m_addr;
	}
	// 获取sockaddr_in6
	const struct sockaddr_in6* getSockAddrIn6() const
	{
		return &m_addr6;
	}
	// 设置ipv4地址
	void setSockAddrInet(const struct sockaddr_in& addr)
	{
		m_addr = addr;
	}
	// 设置ipv6地址
	void setSockAddrInet6(const struct sockaddr_in6& addr6) 
	{
		m_addr6 = addr6;
	}
	// 返回必须IPV4的网络字节序
	uint32_t ipNetEndian() const;
	// 端口的网络字节序，IPV4，IPV6都可以
	uint16_t portNetEndian() const 
	{ 
		return m_addr.sin_port; 
	}
	// 线程安全，从主机名得到IP地址
	static bool resolve(StringArg hostname, InetAddress* result);
	// set IPv6 ScopeID
	void setScopeId(uint32_t scope_id);
	// 重设端口
	void resetPort(uint16_t port, bool ipv6 = false);

private:
	// 网络地址ipv4/ipv6
	union
	{
		struct sockaddr_in m_addr;
		struct sockaddr_in6 m_addr6;
	};
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_INETADDRESS_H_
