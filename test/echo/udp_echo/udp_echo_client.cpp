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
// ֹͣ����
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

    // ��ʹ��connect��block
    // 1.��������socket�������û�п���������£�UDPû������û�г��ַ�����Ϣ��������������ҷ���ֵ��ȷ
    // 2.��������socket������˿���������£�UDP��������Ϣ������ֵ��ȷ
    // 3.��������socket�������û�п���������£�UDP������Ϣֱ�ӷ���-1
    // 4.��������socket������˿���������£�������û�з�����Ϣ��UDP����������Ϣ��������ʱ���ֶ��رշ���ˣ�����������
    // ����˷���Ϣ����������
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

    // UDPΪʲôҪʹ��connect�������·�ɵķ�ʱ�䣬������Կ���
    // https://stackoverflow.com/questions/9741392/can-you-bind-and-connect-both-ends-of-a-udp-connection
    // ʹ��connect��block
    // 1.��������socket����������connect�������û�п���������£�UDPû������û�г��ַ�����Ϣ��������������ҷ���ֵ��ȷ
    // ����2�ο����ڶ����Ƿ񱨴������û�б���
    // 2.��������socket����������connect������˿���������£�UDP��������Ϣ������ֵ��ȷ
    // 3.��������socket����������connect�������û�п���������£�UDP������Ϣֱ�ӷ���-1
    // 4.��������socket����������connect������˿���������£�������û�з�����Ϣ��UDP����������Ϣ��������ʱ���ֶ��رշ���ˣ�����������
    // ����˷���Ϣ����������
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

    // 1.���Է�����socket����������connect�������û�п���������£�����ֵ��ȷ
    // 2.��������socket����������connect������˿���������£�UDP��������Ϣ������ֵ��ȷ
    // 3.��������socket����������connect�������û�п���������£�UDP������Ϣֱ�ӷ���-1
    // 4.��������socket����������connect������˿���������£�������û�з�����Ϣ��UDP������������Ϣ
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

    // ��������û�п����������ͣ�ֱ�ӽ��գ�����-1
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
    
    // ������˼�������û������������£�����Ϣ�Ժ�Ż��յ��ɶ���Ϣ�ص���
    // ��Ȼ�ײ㷵�ص�-1����ϸ����Ҳ�������
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
        // ������Ϣ
        const void* sendBuf = reinterpret_cast<const void*>(line.data());
        ns = sockets::sz_socket_write(sock.fd(), sendBuf, line.size());
        LOG_INFO << "sz_socket_write rst: " << ns;
    }

	return 0;
}
