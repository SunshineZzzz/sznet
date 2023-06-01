#include "KcpWithTcpServer.h"
#include "Acceptor.h"
#include "UdpOps.h"
#include "KcpTcpEventLoop.h"
#include "KcpTcpEventLoopThread.h"
#include "KcpTcpEventLoopThreadPool.h"
#include "../log/Logging.h"

namespace sznet
{

namespace net
{

KcpWithTcpServer::KcpWithTcpServer(KcpTcpEventLoop* loop, const InetAddress& listenAddr, const std::vector<InetAddress>& udpListenAddrs, const string& nameArg, int kcpMode, Option option) :
	m_loop(CHECK_NOTNULL(loop)),
	m_ipPort(listenAddr.toIpPort()),
	m_name(nameArg),
	m_acceptor(new Acceptor(loop, listenAddr, option == kReusePort)),
	m_threadPool(new KcpTcpEventLoopThreadPool(loop, udpListenAddrs, m_name)),
	m_nextConnId(1),
	m_udpListenAddrs(udpListenAddrs),
	m_tcpCodec(std::bind(&KcpWithTcpServer::tcpMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
	m_kcpMode(kcpMode)
{
	m_acceptor->setNewConnectionCallback(std::bind(&KcpWithTcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

KcpWithTcpServer::~KcpWithTcpServer()
{
	m_loop->assertInLoopThread();
	LOG_TRACE << "KcpWithTcpServer::~KcpWithTcpServer [" << m_name << "] destructing";

	for (auto& item : m_connections)
	{
		TcpConnectionPtr conn(item.second);
		item.second.reset();
		conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
	}
}

void KcpWithTcpServer::setThreadNum(int numThreads)
{
	assert(0 <= numThreads);
	assert(m_udpListenAddrs.size() == numThreads);
	m_threadPool->setThreadNum(numThreads);
}

void KcpWithTcpServer::start()
{
	if (m_started.exchange(1) == 0)
	{
		m_threadPool->start(std::bind(&KcpWithTcpServer::onThreadInitCallback, this, std::placeholders::_1));
		assert(!m_acceptor->listenning());
		// listen操作必须确保在m_loop所在线程中执行
		m_loop->runInLoop(std::bind(&Acceptor::listen, get_pointer(m_acceptor)));
	}
}

void KcpWithTcpServer::newConnection(sockets::sz_sock sockfd, const InetAddress& peerAddr)
{
	m_loop->assertInLoopThread();
	KcpTcpEventLoop* ioLoop = m_threadPool->getNextLoop();
	char buf[256];
	uint32_t connId = m_nextConnId++;
	snprintf(buf, sizeof(buf), "-tcp-%s#%d", m_ipPort.c_str(), connId);
	string connName = m_name + buf;

	LOG_INFO << "KcpWithTcpServer::newConnection [" << m_name
		<< "] - new connection [" << connName
		<< "] from " << peerAddr.toIpPort();
	InetAddress localAddr(sockets::sz_sock_getlocaladdr(sockfd));
	// 创建连接
	// FIXME poll with zero timeout to double confirm the new connection
	// FIXME use make_shared if necessary
	TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, connId, sockfd, localAddr, peerAddr));
	m_connections[connId] = conn;
	conn->setConnectionCallback(std::bind(&KcpWithTcpServer::tcpConnection, this, std::placeholders::_1, ioLoop));
	conn->setMessageCallback(std::bind(&LengthHeaderCodec<>::onMessage, &m_tcpCodec, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	// FIXME: unsafe
	conn->setCloseCallback(std::bind(&KcpWithTcpServer::removeConnection, this, std::placeholders::_1));
	ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void KcpWithTcpServer::removeConnection(const TcpConnectionPtr& conn)
{
	// FIXME: unsafe
	m_loop->runInLoop(std::bind(&KcpWithTcpServer::removeConnectionInLoop, this, conn));
}

void KcpWithTcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
	m_loop->assertInLoopThread();
	LOG_INFO << "KcpWithTcpServer::removeConnectionInLoop [" << m_name
		<< "] - connection " << conn->name() << " - " << conn->id();
	size_t n = m_connections.erase(conn->id());
	(void)n;
	assert(n == 1);
	KcpTcpEventLoop* ioLoop = down_cast<KcpTcpEventLoop*>(conn->getLoop());
	ioLoop->queueInLoop(std::bind(&KcpTcpEventLoop::removeTcpConnectionInLoop, ioLoop, conn));
}

void KcpWithTcpServer::tcpConnection(const TcpConnectionPtr& conn, KcpTcpEventLoop* pKcpTcploop)
{
	if (!conn->connected())
	{
		return;
	}

	assert(pKcpTcploop == conn->getLoop());
	Buffer& tmpBuffer = pKcpTcploop->getTmpBuffer();
	tmpBuffer.retrieveAll();
	KcpTcpEventLoop::TcpConnectionMap& tcpConnections = pKcpTcploop->getTcpConnections();
	KcpTcpEventLoop::KcpConnectionMap& kcpConnections = pKcpTcploop->getKcpConnections();
	KcpTcpEventLoop::cryptConvMap& crypts = pKcpTcploop->getCrypts();
	const uint16_t udpPort = pKcpTcploop->getUdpListenPort();
	uint32_t conv = conn->id();
	auto tFindRet = tcpConnections.find(conv);
	assert(tFindRet == tcpConnections.end());
	auto kFindRet = kcpConnections.find(conv);
	assert(kFindRet == kcpConnections.end());
	int secretId = pKcpTcploop->genSecret();
	// 通过TCP连接发送初始化消息
	tmpBuffer.appendInt16(udpPort);
	tmpBuffer.appendInt32(conv);
	tmpBuffer.appendInt32(secretId);
	m_tcpCodec.send(conn.get(), tmpBuffer, sizeof(uint16_t) + sizeof(uint32_t) + sizeof(int32_t));
	// 映射
	tcpConnections[conv] = conn;
	crypts[conv] = secretId;
}

void KcpWithTcpServer::tcpMessage(const sznet::net::TcpConnectionPtr& conn, const sznet::string& message, sznet::Timestamp)
{
	LOG_INFO << "KcpWithTcpServer::tcpMessage: " << message << " - " << conn->name() << " - " << conn->id();
}

void KcpWithTcpServer::onThreadInitCallback(KcpTcpEventLoop* loop)
{
	if (m_kcpConnectionCallback)
	{
		loop->setKcpConnectionCallback(m_kcpConnectionCallback);
	}
	if (m_kcpMessageCallback)
	{
		loop->setKcpMessageCallback(m_kcpMessageCallback);
	}
	loop->setKcpWriteCompleteCallback(m_kcpWriteCompleteCallback);
	loop->setKcpMode(m_kcpMode);
	if (m_threadInitCallback)
	{
		m_threadInitCallback(loop);
	}
}

} // end namespace net

} // end namespace sznet
