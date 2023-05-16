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
    EchoClient(EventLoop* loop, const InetAddress& listenAddr) : 
        m_loop(loop),
        m_client(loop, listenAddr, "EchoClient"),
        m_codec(std::bind(&EchoClient::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
        m_mutex()
    {
        m_client.setConnectionCallback(
            std::bind(&EchoClient::onConnection, this, std::placeholders::_1));
        m_client.setMessageCallback(std::bind(&LengthHeaderCodec::onMessage, &m_codec, 
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
            }
        }
    }
    void onMessage(const sznet::net::TcpConnectionPtr& conn, const sznet::string& message, sznet::Timestamp receiveTime)
    {
        LOG_INFO << "client recv: " << message;
    }

public:
    // 连接所属loop
    EventLoop* m_loop;
    // 客户端对象
    TcpClient m_client;
    // 互斥量
    sznet::MutexLock m_mutex;
    // 4字节编解码器
    LengthHeaderCodec m_codec;
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
    if (argc <= 1)
    {
        printf("Usage: %s host_ip\n", argv[0]);
        return 0;
    }

    signal(SIGINT, StopSignal);
    signal(SIGTERM, StopSignal);

    EventLoopThread threadLoop;
    EventLoop* pLoop = threadLoop.startLoop();

    InetAddress serverAddr(argv[1], 2023);

    EchoClient client(pLoop, serverAddr);
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
        // 发送消息
        client.write(line);
    }

    TcpConnectionPtr conn = client.m_client.connection();
    while (conn.use_count() > 1)
    {
        sznet::sz_sleep(1 * 1000);
    }

    return 0;
}

