// Comment: �����ַʵ��

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

// �����ַ�࣬��sockaddr_in��һ����װ��ʵ���˸������sockaddr_in�Ĳ���
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

	// ����Э����
	unsigned short family() const 
	{ 
		return m_addr.sin_family; 
	}
	// ����ip���ַ���
	string toIp() const;
	// ����ip�Ͷ˿ڵ��ַ���
	string toIpPort() const;
	// ���ض˿�
	uint16_t port() const;
	// ��ȡsockaddr
	const struct sockaddr* getSockAddr() const 
	{ 
		return sockets::sz_sockaddr_cast(&m_addr6); 
	}
	// ��ȡsockaddr_in
	const struct sockaddr_in* getSockAddrIn() const
	{
		return &m_addr;
	}
	// ��ȡsockaddr_in6
	const struct sockaddr_in6* getSockAddrIn6() const
	{
		return &m_addr6;
	}
	// ����ipv4��ַ
	void setSockAddrInet(const struct sockaddr_in& addr)
	{
		m_addr = addr;
	}
	// ����ipv6��ַ
	void setSockAddrInet6(const struct sockaddr_in6& addr6) 
	{
		m_addr6 = addr6;
	}
	// ���ر���IPV4�������ֽ���
	uint32_t ipNetEndian() const;
	// �˿ڵ������ֽ���IPV4��IPV6������
	uint16_t portNetEndian() const 
	{ 
		return m_addr.sin_port; 
	}
	// �̰߳�ȫ�����������õ�IP��ַ
	static bool resolve(StringArg hostname, InetAddress* result);
	// set IPv6 ScopeID
	void setScopeId(uint32_t scope_id);
	// ����˿�
	void resetPort(uint16_t port, bool ipv6 = false);

private:
	// �����ַipv4/ipv6
	union
	{
		struct sockaddr_in m_addr;
		struct sockaddr_in6 m_addr6;
	};
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_INETADDRESS_H_
