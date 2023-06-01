#include <sznet/net/TcpServer.h>
#include <sznet/log/Logging.h>
#include <sznet/thread/Thread.h>
#include <sznet/net/EventLoop.h>
#include <sznet/net/InetAddress.h>
#include <sznet/process/Process.h>
#include <sznet/net/Codec.h>

#include <atomic>
#include <utility>
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
    EchoServer(EventLoop* loop, const InetAddress& listenAddr, int mode = 1) : 
        m_server(loop, listenAddr, "EchoServer"),
        m_transferred(0),
        m_receivedMessages(0),
        m_oldCounter(0),
        m_startTime(Timestamp::now()),
        m_codec(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
        m_mode(mode)
    {
        m_server.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        m_server.setMessageCallback(std::bind(&LengthHeaderCodec<>::onMessage, &m_codec,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        m_server.setThreadNum(numThreads);
        if (m_mode == 1)
        {
            loop->runEvery(3.0, std::bind(&EchoServer::printThroughput, this));
        }
    }

    void start()
    {
        LOG_INFO << "starting " << numThreads << " threads.";
        m_server.start();
    }

private:
    void onConnection(const TcpConnectionPtr& conn)
    {
        LOG_TRACE << conn->peerAddress().toIpPort() << " -> "
            << conn->localAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
        conn->setTcpNoDelay(true);
    }
    void onMessage(const sznet::net::TcpConnectionPtr& conn, const sznet::string& message, sznet::Timestamp receiveTime)
    {
        if (m_mode == 1)
        {
            LOG_INFO << "server recv: " << message;
        }
        m_codec.send(get_pointer(conn), message);
        size_t len = message.size();
        m_transferred.fetch_add(len);
        ++m_receivedMessages;
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

    // TCP服务器
    TcpServer m_server;
    // 总接收到数据字节数
    std::atomic<int64_t> m_transferred;
    // timer内接收到数据次数
    std::atomic<int64_t> m_receivedMessages;
    // timer上次收到的字节数
    int64_t m_oldCounter;
    // 两次timer的差值
    Timestamp m_startTime;
    // 编解码器
    LengthHeaderCodec<> m_codec;
    // 1 - echo, others - RTT
    int m_mode;
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
    if (argc <= 2)
    {
        printf("Usage: %s thread_num mode(1-echo, others-RTT)\n", argv[0]);
        return 0;
    }

    numThreads = atoi(argv[1]);
    numThreads = (numThreads == 0 ? 2 : numThreads);
    int mode = atoi(argv[2]);

    EventLoop loop;
    g_pMainLoopsRun = &loop;
    InetAddress listenAddr(2023);
    EchoServer server(&loop, listenAddr, mode);

    server.start();

    loop.loop();

    return 0;
}

