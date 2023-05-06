// Comment: socketʵ��

#ifndef _SZNET_NET_SOCKET_H_
#define _SZNET_NET_SOCKET_H_

#include "../NetCmn.h"
#include "../base/NonCopyable.h"
#include "SocketsOps.h"

namespace sznet
{

namespace net
{

// �����ַ
class InetAddress;

// socket������fd�ķ�װ
class Socket : NonCopyable
{
public:
	explicit Socket(sockets::sz_sock sockfd): 
		m_sockfd(sockfd)
	{
	}
	~Socket();

	// ����fd
	sockets::sz_sock fd() const
	{ 
		return m_sockfd;
	}
	// �������ַ
	void bindAddress(const InetAddress& localaddr);
	// ����
	void listen();
	// ���������ӣ��ɹ�����non-blocking��close-on-exec���Ե�connfd
	sockets::sz_sock accept(InetAddress* peeraddr);
	// �ر�write��
	void shutdownWrite();
	// �Ƿ�ʹ�� Nagle �㷨�����ӳٳ���ر�
	void setTcpNoDelay(bool on);
	// �Ƿ�SO_REUSEADDR
	void setReuseAddr(bool on);
	// �Ƿ�SO_REUSEPORT
	void setReusePort(bool on);
	// �Ƿ�������ϵ��˱���TCP�ղ���FIN����ñ�д�߼�������
	void setKeepAlive(bool on);

private:
	// socket fd
	const sockets::sz_sock m_sockfd;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_SOCKET_H_
