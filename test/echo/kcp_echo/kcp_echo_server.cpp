#include <sznet/net/KcpWithTcpServer.h>
#include <sznet/net/EventLoopThread.h>
#include <sznet/net/KcpTcpEventLoop.h>
#include <sznet/net/KcpConnection.h>

#include <stdio.h>
#include <signal.h>

#if SZ_COMPILER_MSVC
#	pragma comment(lib, "sznet.lib")
#endif

using namespace sznet;
using namespace sznet::net;

int numThreads = 0;
EventLoop* g_pMainLoopsRun = nullptr;
// 停止函数
void StopSignal(int sig)
{
    if (g_pMainLoopsRun)
    {
        g_pMainLoopsRun->quit();
    }
    (void)sig;
}

class EchoServer
{
public:
    EchoServer(KcpTcpEventLoop* loop, const std::vector<InetAddress>& udpListenAddrs, const InetAddress& listenAddr) :
        m_server(loop, listenAddr, udpListenAddrs, "EchoServer"),
        m_transferred(0),
        m_receivedMessages(0),
        m_oldCounter(0),
        m_startTime(Timestamp::now()),
        m_codec(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    {
        m_server.setKcpConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        m_server.setKcpMessageCallback(std::bind(&LengthHeaderCodec<KcpConnectionPtr>::onMessage, &m_codec,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        m_server.setThreadNum(numThreads);
        loop->runEvery(3.0, std::bind(&EchoServer::printThroughput, this));
    }

    void start()
    {
        LOG_INFO << "starting " << numThreads << " threads.";
        m_server.start();
    }

private:
    void onConnection(const KcpConnectionPtr& conn)
    {
        LOG_INFO << conn->name() << " - "  << conn->peerAddress().toIpPort() 
            << " -> " << conn->localAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
    }

    void onMessage(const sznet::net::KcpConnectionPtr& conn, const sznet::string& message, sznet::Timestamp receiveTime)
    {
        LOG_INFO << "server recv: " << message;

        size_t len = message.size();
        m_transferred.fetch_add(len);
        ++m_receivedMessages;
        m_codec.send(get_pointer(conn), message);
        (void)receiveTime;
    }

    // 吞吐打印
    void printThroughput()
    {
        Timestamp endTime = Timestamp::now();
        int64_t newCounter = m_transferred;
        int64_t bytes = newCounter - m_oldCounter;
        int64_t msgs = m_receivedMessages.exchange(0);
        double time = timeDifference(endTime, m_startTime);
        printf("%4.3f MiB/s %4.3f Ki Msgs/s %6.2f bytes per msg\n",
            static_cast<double>(bytes) / time / 1024 / 1024,
            static_cast<double>(msgs) / time / 1024,
            static_cast<double>(bytes) / static_cast<double>(msgs));

        m_oldCounter = newCounter;
        m_startTime = endTime;
    }

    // KCP服务器
    KcpWithTcpServer m_server;
    // 总接收到数据字节数
    std::atomic<int64_t> m_transferred;
    // timer内接收到数据次数
    std::atomic<int64_t> m_receivedMessages;
    // timer上次收到的字节数
    int64_t m_oldCounter;
    // 两次timer的差值
    Timestamp m_startTime;
    // 编解码器
    LengthHeaderCodec<KcpConnectionPtr> m_codec;
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

    signal(SIGINT, StopSignal);
    signal(SIGTERM, StopSignal);

    LOG_INFO << "pid = " << sz_getpid() << ", tid = " << CurrentThread::tid();
    if (argc > 1)
    {
        numThreads = atoi(argv[1]);
    }
    numThreads = (numThreads == 0 ? 2 : numThreads);
    InetAddress tcpListenAddr(2023);
    std::vector<InetAddress> udpListenAddr;
    for (int i = 1; i <= numThreads; ++i)
    {
        udpListenAddr.emplace_back(2023 + i);
    }

    KcpTcpEventLoop loop(tcpListenAddr, "base", 0);
    g_pMainLoopsRun = &loop;
    EchoServer server(&loop, udpListenAddr, tcpListenAddr);

    server.start();

    loop.loop();

    return 0;
}

