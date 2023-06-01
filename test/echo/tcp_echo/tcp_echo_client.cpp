#include <sznet/net/TcpClient.h>

#include <sznet/log/Logging.h>
#include <sznet/thread/Thread.h>
#include <sznet/net/EventLoop.h>
#include <sznet/net/EventLoopThread.h>
#include <sznet/net/InetAddress.h>
#include <sznet/net/Codec.h>
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
    EchoClient(EventLoop* loop, const InetAddress& listenAddr, int mode = 1) : 
        m_loop(loop),
        m_client(loop, listenAddr, "EchoClient"),
        m_codec(std::bind(&EchoClient::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
        m_mutex(),
        m_mode(mode)
    {
        m_client.setConnectionCallback(
            std::bind(&EchoClient::onConnection, this, std::placeholders::_1));
        m_client.setMessageCallback(std::bind(&LengthHeaderCodec<>::onMessage, &m_codec, 
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        // m_client.enableRetry();
    }

    // 发起连接
    void connect()
    {
        m_client.connect();
    }
    // 断开连接
    void disconnect()
    {
        m_client.disconnect();
    }
    // 发送消息
    void write(const sznet::StringPiece& message)
    {
        sznet::MutexLockGuard lock(m_mutex);
        m_codec.send(get_pointer(m_client.connection()), message);
    }

private:
    // 连接 / 断连通知
    void onConnection(const TcpConnectionPtr& conn)
    {
        LOG_TRACE << conn->localAddress().toIpPort() << " -> "
            << conn->peerAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
        {
            sznet::MutexLockGuard lock(m_mutex);
            if (conn->connected())
            {
                conn->setTcpNoDelay(true);
                if (m_mode == 2)
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
    void onMessage(const sznet::net::TcpConnectionPtr& conn, const sznet::string& message, sznet::Timestamp receiveTime)
    {
        if (m_mode == 1)
        {
            LOG_INFO << "client recv: " << message;
            return;
        }
        uint32_t last = *reinterpret_cast<const uint32_t*>(message.data());
        uint32_t rtt = (static_cast<uint32_t>(Timestamp::now().milliSecondsSinceEpoch()) - last) / 2;
        LOG_INFO << "rtt: " << rtt;
    }
    // timer不断的发送
    void everySendForRtt()
    {
        m_loop->assertInLoopThread();
        if (!m_client.connection()->connected())
        {
            return;
        }
        uint32_t st = static_cast<uint32_t>(Timestamp::now().milliSecondsSinceEpoch());
        m_codec.send(get_pointer(m_client.connection()), reinterpret_cast<const void*>(&st), sizeof(uint32_t));
    }

public:
    // 连接所属loop
    EventLoop* m_loop;
    // 客户端对象
    TcpClient m_client;
    // 互斥量
    sznet::MutexLock m_mutex;
    // 4字节编解码器
    LengthHeaderCodec<> m_codec;
    // 1 - echo, others - RTT
    int m_mode;
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
        printf("Usage: %s host_ip port mode(1-echo, others-RTT)\n", argv[0]);
        return 0;
    }

    signal(SIGINT, StopSignal);
    signal(SIGTERM, StopSignal);

    char* szSvrIp = argv[1];
    uint16_t sSvrPort = static_cast<uint16_t>(atoi(argv[2]));
    int mode = atoi(argv[3]);

    EventLoopThread threadLoop;
    EventLoop* pLoop = threadLoop.startLoop();

    InetAddress serverAddr(szSvrIp, sSvrPort);

    EchoClient client(pLoop, serverAddr, mode);
    client.connect();

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
            client.disconnect();
            break;
        }
        if (mode == 1)
        {
            // 发送消息
            client.write(line);
        }
    }

    TcpConnectionPtr conn = client.m_client.connection();
    while (conn.use_count() > 1)
    {
        sz_sleep(1 * 1000);
    }

    return 0;
}

