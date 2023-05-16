#include <sznet/log/Logging.h>
#include <sznet/net/UdpOps.h>
#include <sznet/process/Process.h>
#include <sznet/thread/CurrentThread.h>
#include <sznet/net/InetAddress.h>
#include <sznet/net/Socket.h>
#include <sznet/net/EventLoop.h>
#include <sznet/net/Channel.h>
#include <sznet/string/StringOpt.h>

#include <signal.h>
#include <string>
#include <iostream>
#include <vector>

#if SZ_COMPILER_MSVC
#	pragma comment(lib, "sznet.lib")
#endif

using namespace sznet;
using namespace sznet::net;

int numThreads = 0;
EventLoop* g_pLoopsRun = nullptr;
// Í£Ö¹º¯Êý
void StopSignal(int sig)
{
    if (g_pLoopsRun)
    {
        g_pLoopsRun->quit();
    }
    (void)sig;
}

void UdpServerReadCallback(sockets::sz_sock sockfd, sznet::Timestamp receiveTime)
{
    char message[1024];
    void* recvBuf = reinterpret_cast<void*>(&message);
    struct sockaddr peerAddr;
    memset(&peerAddr, 0, sizeof(peerAddr));
    
    sz_ssize_t nr = sockets::sz_udp_recv(sockfd, recvBuf, 1024, &peerAddr);
    if (nr < 0)
    {
        LOG_SYSERR << "sz_udp_recv";
        return;
    }

    char addrStr[64];
    sockets::sz_toipport(addrStr, sizeof(addrStr), &peerAddr);
    message[nr] = 0;
    LOG_INFO << "recv: " << message << ", from: " << addrStr;

    const void* sendBuf = const_cast<const void*>(recvBuf);
    int sendBufSize = static_cast<int>(nr);
    nr = sockets::sz_udp_send(sockfd, sendBuf, sendBufSize, &peerAddr);
    LOG_INFO << "echo send: " << message << ", to: " << addrStr;
}

int main(int argc, char* argv[])
{
    sznet::sztool_memcheck();
#ifdef _WIN32
    static int block = 0;
    if (block)
    {
        _CrtSetBreakAlloc(block);
    }
#endif

    signal(SIGINT, StopSignal);
    signal(SIGTERM, StopSignal);

    LOG_INFO << "pid = " << sz_getpid() << ", tid = " << CurrentThread::tid();

    EventLoop loop;
    g_pLoopsRun = &loop;
    InetAddress bindAddr(2023);
    Socket sock(sockets::sz_udp_create(bindAddr.family()));
    sockets::sz_sock_setnonblock(sock.fd(), true);
    sock.bindAddress(bindAddr);
    Channel channel(&loop, sock.fd());
    channel.setReadCallback(std::bind(&UdpServerReadCallback, sock.fd(), std::placeholders::_1));
    channel.enableReading();

    loop.loop();

    return 0;
}
