// Comment: UDP相关接口

#ifndef _SZNET_NET_UDPOPS_H_
#define _SZNET_NET_udpOPS_H_

#include "SocketsOps.h"

namespace sznet
{

namespace net
{

namespace sockets
{

// 创建非阻塞udp socket
sz_sock sz_udp_create(int family);
// 创建非阻塞udp socket
sz_sock sz_udp_createnonblockingordie(int family);
// 接收数据
sz_ssize_t sz_udp_recv(sz_sock sock, void* buf, int size, struct sockaddr* addr);
// 发送数据
sz_ssize_t sz_udp_send(sz_sock sock, const void* buf, int size, const struct sockaddr* addr);

} // end namespace sockets

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_UDPOPS_H_
