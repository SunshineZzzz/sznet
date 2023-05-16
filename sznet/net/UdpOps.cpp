#include "UdpOps.h"
#include "../log/Logging.h"
#include "../base/Types.h"

namespace sznet
{

namespace net
{

namespace sockets
{

sz_sock sz_udp_create(int family)
{
	sz_sock sockfd = ::socket(family, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd < 0)
	{
		LOG_SYSFATAL << "sockets::sz_udp_create";
	}

	return sockfd;
}

sz_sock sz_udp_createnonblockingordie(int family)
{
	sz_sock sockfd = sz_udp_create(family);
	sz_setnonblockandcloseonexec(sockfd);

	return sockfd;
}

sz_ssize_t sz_udp_recv(sz_sock sock, void* buf, int size, struct sockaddr* addr)
{
#if defined(SZ_OS_LINUX)
	void* recvBuf = buf;
	size_t recvBufSize = implicit_cast<size_t>(size);
	socklen_t lenAddr = sizeof(*addr);
#else
	char* recvBuf = reinterpret_cast<char*>(buf);
	int recvBufSize = size;
	int lenAddr = sizeof(*addr);
#endif

	return ::recvfrom(sock, recvBuf, recvBufSize, 0, addr, &lenAddr);
}

sz_ssize_t sz_udp_send(sz_sock sock, const void* buf, int size, const struct sockaddr* addr)
{
#if defined(SZ_OS_LINUX)
	const void* sendBuf = buf;
	size_t sendBufSize = implicit_cast<size_t>(size);
	socklen_t lenAddr = sizeof(*addr);
#else
	const char* sendBuf = reinterpret_cast<const char*>(buf);
	int sendBufSize = size;
	int lenAddr = sizeof(*addr);
#endif

	return ::sendto(sock, sendBuf, sendBufSize, 0, addr, lenAddr);
}

} // end namespace sockets

} // end namespace net

} // end namespace sznet
