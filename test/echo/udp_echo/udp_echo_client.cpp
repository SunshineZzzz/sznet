#include <sznet/log/Logging.h>
#include <sznet/net/UdpOps.h>
#include <sznet/process/Process.h>
#include <sznet/thread/CurrentThread.h>
#include <sznet/net/InetAddress.h>
#include <sznet/net/Socket.h>
#include <sznet/net/EventLoop.h>
#include <sznet/net/EventLoopThread.h>
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

bool g_bRunTest = false;
bool g_IsRun = true;
// 停止函数
void StopSignal(int sig)
{
    g_IsRun = false;
    (void)sig;
}

void HaveATest(Socket& sock, const InetAddress& serverAddr)
{
    struct sockaddr * serverSockAddr = const_cast<struct sockaddr*>(serverAddr.getSockAddr());
    sockets::sz_sock csock = sock.fd();
    const char* msg = "hello";
    const void* sendMsg = reinterpret_cast<const void*>(msg);
    int sendLen = static_cast<int>(strlen(msg));
    char recv[10] = {};
    void* recvBuf = reinterpret_cast<void*>(recv);
    int recvLen = 10;
    sz_ssize_t rst = 0;

    // 不使用connect，block
    // 1.测试阻塞socket，服务端没有开启的情况下，UDP没有流控没有出现发送消息阻塞的情况，并且返回值正确
    // 2.测试阻塞socket，服务端开启的情况下，UDP正常发消息，返回值正确
    // 3.测试阻塞socket，服务端没有开启的情况下，UDP接收消息直接返回-1
    // 4.测试阻塞socket，服务端开启的情况下，服务器没有发送消息，UDP阻塞接收消息，如果这个时候手动关闭服务端，还是阻塞，
    // 服务端发消息，正常接收
    do
    {
        LOG_INFO << "TEST1 BEGIN, no connect, block";
        rst = sockets::sz_udp_send(csock, sendMsg, sendLen, serverSockAddr);
        LOG_INFO << "sz_udp_send rst: " << rst;

        rst = sockets::sz_udp_recv(csock, recvBuf, recvLen, serverSockAddr);
        if (rst < 0)
        {
            LOG_SYSERR << "sz_udp_recv error: " << rst;
        }
        else
        {
            recv[rst] = 0;
            LOG_INFO << "sz_udp_recv rst: " << rst << ", recv msg: " << recv;
        }

        LOG_INFO << "TEST1 END, no connect, block";
    } while (false);

    // UDP为什么要使用connect，避免查路由的费时间，具体可以看：
    // https://stackoverflow.com/questions/9741392/can-you-bind-and-connect-both-ends-of-a-udp-connection
    // 使用connect，block
    // 1.测试阻塞socket，主动调用connect，服务端没有开启的情况下，UDP没有流控没有出现发送消息阻塞的情况，并且返回值正确
    // 调用2次看看第二次是否报错，结果并没有报错
    // 2.测试阻塞socket，主动调用connect，服务端开启的情况下，UDP正常发消息，返回值正确
    // 3.测试阻塞socket，主动调用connect，服务端没有开启的情况下，UDP接收消息直接返回-1
    // 4.测试阻塞socket，主动调用connect，服务端开启的情况下，服务器没有发送消息，UDP阻塞接收消息，如果这个时候手动关闭服务端，还是阻塞，
    // 服务端发消息，正常接收
    do
    {
        LOG_INFO << "TEST2 BEGIN, connect, block";

        sockets::sz_sock_connect(csock, serverSockAddr);
        rst = sockets::sz_udp_send(csock, sendMsg, sendLen, serverSockAddr);
        if (rst < 0)
        {
            LOG_SYSERR << "first call sz_udp_recv error: " << rst;
            return;
        }
        LOG_INFO << "first call sz_udp_send rst: " << rst;

        rst = sockets::sz_udp_send(csock, sendMsg, sendLen, serverSockAddr);
        if (rst < 0)
        {
            LOG_SYSERR << "second call sz_udp_recv error: " << rst;
            return;
        }
        LOG_INFO << "second call sz_udp_send rst: " << rst;

        rst = sockets::sz_udp_recv(csock, recvBuf, recvLen, serverSockAddr);
        if (rst < 0)
        {
            LOG_SYSERR << "sz_udp_recv error: " << rst;
        }
        else
        {
            recv[rst] = 0;
            LOG_INFO << "sz_udp_recv rst: " << rst << ", recv msg: " << recv;
        }

        LOG_INFO << "TEST2 END, connect, block";
    } while (false);

    // 1.测试非阻塞socket，主动调用connect，服务端没有开启的情况下，返回值正确
    // 2.测试阻塞socket，主动调用connect，服务端开启的情况下，UDP正常发消息，返回值正确
    // 3.测试阻塞socket，主动调用connect，服务端没有开启的情况下，UDP接收消息直接返回-1
    // 4.测试阻塞socket，主动调用connect，服务端开启的情况下，服务器没有发送消息，UDP不阻塞接收消息
    do
    {
        LOG_INFO << "TEST3 BEGIN, connect, noblock";

        sockets::sz_sock_setnonblock(csock, true);
        sockets::sz_sock_connect(csock, serverSockAddr);
        rst = sockets::sz_udp_send(csock, sendMsg, sendLen, serverSockAddr);
        if (rst < 0)
        {
            LOG_SYSERR << "call sz_udp_recv error: " << rst;
            return;
        }
        LOG_INFO << "call sz_udp_send rst: " << rst;

        rst = sockets::sz_udp_recv(csock, recvBuf, recvLen, serverSockAddr);
        if (rst < 0)
        {
            if (sockets::sz_wouldblock())
            {
                LOG_INFO << "async recv";
            }
            else
            {
                LOG_SYSERR << "sz_udp_recv error: " << rst;
            }
        }
        else
        {
            recv[rst] = 0;
            LOG_INFO << "sz_udp_recv rst: " << rst << ", recv msg: " << recv;
        }

        LOG_INFO << "TEST2 END, connect, block";
    } while (false);

    // 服务器端没有开启，不发送，直接接收，错误-1
    do
    {
        LOG_INFO << "TEST4 BEGIN, no connect, block";

        rst = sockets::sz_udp_recv(csock, recvBuf, recvLen, serverSockAddr);
        if (rst < 0)
        {
            LOG_SYSERR << "sz_udp_recv error: " << rst;
        }
        else
        {
            recv[rst] = 0;
            LOG_INFO << "sz_udp_recv rst: " << rst << ", recv msg: " << recv;
        }

        LOG_INFO << "TEST4 END, no connect, block";
    } while (false);
}

void UdpClientReadCallback(sockets::sz_sock sockfd, sznet::Timestamp receiveTime)
{
    char recvBuf[1024];
    sz_ssize_t nr = sockets::sz_read(sockfd, recvBuf, sizeof(recvBuf));

    if (nr < 0)
    {
        LOG_SYSERR << "sz_read";
        return;
    }

    recvBuf[nr] = 0;

    LOG_INFO << "recv: " << recvBuf;
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

    LOG_INFO << "pid = " << sz_getpid() << ", tid = " << CurrentThread::tid();
    if (argc <= 0)
    {
        printf("Usage: %s host_ip\n", argv[0]);
        return 0;
    }

    signal(SIGINT, StopSignal);
    signal(SIGTERM, StopSignal);

    InetAddress serverAddr(argv[1], 2023);
    Socket sock(sockets::sz_udp_create(serverAddr.family()));

    if (g_bRunTest)
    {
        HaveATest(sock, serverAddr);
        return 0;
    }
    
    // 蛮有意思，服务端没有启动的情况下，发消息以后才会收到可读消息回调，
    // 当然底层返回的-1，仔细想想也蛮合理的
    sockets::sz_sock_setnonblock(sock.fd(), true);
    int ret = sockets::sz_sock_connect(sock.fd(), serverAddr.getSockAddr());
    if (ret < 0)
    {
        LOG_SYSFATAL << "::connect";
    }

    EventLoopThread threadLoop;
    EventLoop* pLoop = threadLoop.startLoop();
    Channel channel(pLoop, sock.fd());
    channel.setReadCallback(std::bind(&UdpClientReadCallback, sock.fd(), std::placeholders::_1));
    pLoop->runInLoop(std::bind(&Channel::enableReading, &channel));

    std::string line;
    sz_ssize_t ns = 0;
    while (std::getline(std::cin, line) && g_IsRun)
    {
        std::vector<sznet::StringPiece> tokens;
        sznet::sz_split(line, tokens);
        if (tokens.empty())
        {
            continue;
        }
        if (tokens[0] == "quit")
        {
            break;
        }
        // 发送消息
        const void* sendBuf = reinterpret_cast<const void*>(line.data());
        ns = sockets::sz_socket_write(sock.fd(), sendBuf, line.size());
        LOG_INFO << "sz_socket_write rst: " << ns;
    }

	return 0;
}
