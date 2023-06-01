#include <sznet/net/KcpConnection.h>
#include <sznet/net/KcpWithTcpClient.h>
#include <sznet/net/EventLoopThread.h>
#include <sznet/net/InetAddress.h>
#include <sznet/net/Codec.h>
#include <sznet/log/Logging.h>
#include <sznet/thread/Thread.h>
#include <sznet/string/StringOpt.h>
#include <sznet/time/Time.h>

#include <utility>
#include <iostream>
#include <string>
#include <stdio.h>
#include <signal.h>

#if SZ_COMPILER_MSVC
#	pragma comment(lib, "sznet.lib")
#endif

using namespace sznet;
using namespace sznet::net;

bool g_IsRun = true;
// 停止函数
void StopSignal(int sig)
{
    g_IsRun = false;
    (void)sig;
}

class EchoClient : NonCopyable
{
public:
    EchoClient(EventLoop* loop, const InetAddress& tcplistenAddr, int mode = 1, int kcpMode = 1) :
        m_loop(loop),
        m_client(loop, tcplistenAddr, "EchoClient", kcpMode),
        m_kcpCodec(std::bind(&EchoClient::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
        m_mode(mode),
        m_kcpMode(kcpMode)
    {
        m_client.setKcpConnectionCallback(std::bind(&EchoClient::onConnection, this, std::placeholders::_1));
        m_client.setKcpMessageCallback(std::bind(&LengthHeaderCodec<KcpConnectionPtr>::onMessage, &m_kcpCodec, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }

    // 发起连接TCP
    void connectTcp()
    {
        m_client.connectTcp();
    }
    // 断开连接TCP
    void disconnectTcp()
    {
        m_client.disconnectTcp();
    }
    // 发送消息
    void write(const sznet::StringPiece& message)
    {
        sznet::MutexLockGuard lock(m_mutex);
        if (m_client.kcpConnected())
        {
            m_kcpCodec.send(get_pointer(m_client.kcpConnection()), message);
        }
    }

private:
    // 连接/断连通知
    void onConnection(const KcpConnectionPtr& conn)
    {
        LOG_INFO << conn->name() << conn->localAddress().toIpPort() 
            << " -> " << conn->peerAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
        {
            sznet::MutexLockGuard lock(m_mutex);
            if (conn->connected())
            {
                if (m_mode == 1)
                {
                    m_everySendForRttTimer = m_loop->runEvery(1.0, std::bind(&EchoClient::everySendForRtt, this));
                }
            }
            else
            {
                m_loop->cancel(m_everySendForRttTimer);
            }
        }
    }
    // 消息
    void onMessage(const KcpConnectionPtr& conn, const sznet::string& message, sznet::Timestamp receiveTime)
    {
        if (m_mode == 1)
        {
            LOG_INFO << "client recv: " << message;
            return;
        }
        uint32_t last = *reinterpret_cast<const uint32_t*>(message.data());
        uint32_t cur = static_cast<uint32_t>(Timestamp::now().milliSecondsSinceEpoch());
        uint32_t rtt = (cur - last) / 2;
        LOG_INFO << "rtt: " << rtt;
    }
    // timer不断的发送
    void everySendForRtt()
    {
        m_loop->assertInLoopThread();
        if (!m_client.kcpConnected())
        {
            return;
        }
        uint32_t st = static_cast<uint32_t>(Timestamp::now().milliSecondsSinceEpoch());
        m_kcpCodec.send(get_pointer(m_client.kcpConnection()), reinterpret_cast<const void*>(&st), sizeof(uint32_t));
    }

public:
    // 连接所属loop
    EventLoop* m_loop;
    // 客户端对象
    KcpWithTcpClient m_client;
    // 互斥量
    sznet::MutexLock m_mutex;
    // 4字节编解码器
    LengthHeaderCodec<KcpConnectionPtr> m_kcpCodec;
    // 1 - echo, others - RTT
    int m_mode;
    // 1 - normal, others - quick
    int m_kcpMode;
    // 便于计算RTT
    TimerId m_everySendForRttTimer;
};

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
    if (argc <= 3)
    {
        printf("Usage: %s host_ip mode(1-echo, others-RTT), kcp_mode(1-normal, others-quick)\n", argv[0]);
        return 0;
    }

    signal(SIGINT, StopSignal);
    signal(SIGTERM, StopSignal);

    char* szSvrIp = argv[1];
    int mode = atoi(argv[2]);
    int kcpMode = atoi(argv[3]);

    EventLoopThread threadLoop;
    EventLoop* pLoop = threadLoop.startLoop();

    InetAddress serverAddr(szSvrIp, 2023);

    EchoClient client(pLoop, serverAddr, mode, kcpMode);

    client.connectTcp();

    std::string line;
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
            client.disconnectTcp();
            break;
        }
        if (mode == 1)
        {
            // 发送消息
            client.write(line);
        }
    }

    KcpConnectionPtr conn = client.m_client.kcpConnection();
    while (conn.use_count() > 1)
    {
        sznet::sz_sleep(1 * 1000);
    }

    return 0;
}

