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
#include <vector>
#include <numeric>
#include <chrono>

#include <stdio.h>
#include <signal.h>

#if SZ_COMPILER_MSVC
#	pragma comment(lib, "sznet.lib")
#endif

using namespace sznet;
using namespace sznet::net;

bool g_IsRun = true;
// ֹͣ����
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

    // ��������
    void connect()
    {
        m_client.connect();
    }
    // �Ͽ�����
    void disconnect()
    {
        m_client.disconnect();
    }
    // ������Ϣ
    void write(const sznet::StringPiece& message)
    {
        sznet::MutexLockGuard lock(m_mutex);
        m_codec.send(get_pointer(m_client.connection()), message);
    }

private:
    // ���� / ����֪ͨ
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
    // ��Ϣ
    void onMessage(const sznet::net::TcpConnectionPtr& conn, const sznet::string& message, sznet::Timestamp receiveTime)
    {
        if (m_mode == 1)
        {
            LOG_INFO << "client recv: " << message;
            return;
        }
        int64_t lastTime = *reinterpret_cast<const int64_t*>(message.data());
        int64_t nowTime = Timestamp::now().milliSecondsSinceEpoch();
        int64_t rtt = static_cast<int64_t>(nowTime - lastTime);
        LOG_INFO << "rtt: " << rtt ;
    }
    // timer���ϵķ���
    void everySendForRtt()
    {
        m_loop->assertInLoopThread();
        if (!m_client.connection()->connected())
        {
            return;
        }
        int64_t st = Timestamp::now().milliSecondsSinceEpoch();
        m_codec.send(get_pointer(m_client.connection()), reinterpret_cast<const void*>(&st), sizeof(st));
    }

public:
    // ��������loop
    EventLoop* m_loop;
    // �ͻ��˶���
    TcpClient m_client;
    // ������
    sznet::MutexLock m_mutex;
    // 4�ֽڱ������
    LengthHeaderCodec<> m_codec;
    // 1 - echo, others - RTT
    int m_mode;
    // ���ڼ���RTT
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
            // ������Ϣ
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

