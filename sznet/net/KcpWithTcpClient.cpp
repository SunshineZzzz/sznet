#include "KcpWithTcpClient.h"
#include "../log/Logging.h"
#include "Connector.h"
#include "EventLoop.h"
#include "SocketsOps.h"
#include "UdpOps.h"


#include <stdio.h>

namespace sznet
{ 

namespace net
{

namespace detail
{

// tcp�����ڶϿ����ӵĻص�����
void removeTcpConnection(EventLoop* loop, const TcpConnectionPtr& conn)
{
	loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

// tcpֹͣ�������ӣ�1���Ļص�����
void removeTcpConnector(const ConnectorPtr& connector)
{
	(void)connector;
}

// kcp�����ڶϿ����ӵĻص�����
void removeKcpConnection(EventLoop* loop, const KcpConnectionPtr& conn)
{
	loop->queueInLoop(std::bind(&KcpConnection::connectDestroyed, conn));
}

} // namespace detail

KcpWithTcpClient::KcpWithTcpClient(EventLoop* loop, const InetAddress& tcpServerAddr, const string& nameArg, int kcpMode):
	m_loop(CHECK_NOTNULL(loop)),
	m_tcpConnector(new Connector(loop, tcpServerAddr)),
    m_name(nameArg),
    m_kcpConnectionCallback(defaultKcpConnectionCallback),
    m_kcpMessageCallback(defaultKcpMessageCallback),
    m_tcpRetry(false),
    m_tcpConnect(true),
    m_nextConnId(1),
	m_tcpCodec(std::bind(&KcpWithTcpClient::handleTcpMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
	m_udpListenAddr(tcpServerAddr),
	m_kcpMode(kcpMode)
{
	m_tcpConnector->setNewConnectionCallback(std::bind(&KcpWithTcpClient::newTcpConnection, this, std::placeholders::_1));
	// FIXME setConnectFailedCallback
	LOG_INFO << "KcpWithTcpClient::KcpWithTcpClient[" << m_name 
		<< "] - tcp connector " << get_pointer(m_tcpConnector);
}

KcpWithTcpClient::~KcpWithTcpClient()
{
	LOG_INFO << "KcpWithTcpClient::~KcpWithTcpClient[" << m_name 
		<< "] - tcp connector " << get_pointer(m_tcpConnector);
	TcpConnectionPtr tcpConn;
	KcpConnectionPtr kcpConn;
	// TCP & KCP �����Ƿ�KcpWithTcpClient(owner)���Գ���
	bool uniqueTcp = false;
	bool uniqueKcp = false;
	{
		MutexLockGuard lock(m_mutex);
		uniqueTcp = m_tcpConnection.use_count() == 1;
		// TcpClient����Ҫ�����ˣ��ڲ���Ҫ������
		tcpConn = m_tcpConnection;
		uniqueKcp = m_kcpConnection.use_count() == 1;
		kcpConn = m_kcpConnection;
	}
	if (tcpConn)
	{
		assert(m_loop == tcpConn->getLoop());
		// ���������Ƿ���Գ���tcp���ӣ�����Ҫ���ƺ����ˣ���ֹtcp�����ڴ�й¶
		// FIXME: not 100% safe, if we are in different thread
		CloseCallback cb = std::bind(&detail::removeTcpConnection, m_loop, std::placeholders::_1);
		m_loop->runInLoop(std::bind(&TcpConnection::setCloseCallback, tcpConn, cb));
		// ���Գ��У�ֱ��ǿ�ƹر�
		if (uniqueTcp)
		{
			tcpConn->forceClose();
		}
	}
	// tcp�����Ѿ����ͷ��ˣ���ô���ֵģ�
	else
	{
		LOG_INFO << "m_tcpConnector release!!!";
		m_tcpConnector->stop();
		// FIXME: HACK
		m_loop->runAfter(1, std::bind(&detail::removeTcpConnector, m_tcpConnector));
	}
	if (kcpConn)
	{
		assert(m_loop == kcpConn->getLoop());
		// ���������Ƿ���Գ���tcp���ӣ�����Ҫ���ƺ����ˣ���ֹtcp�����ڴ�й¶
		// FIXME: not 100% safe, if we are in different thread
		KcpCloseCallback cb = std::bind(&detail::removeKcpConnection, m_loop, std::placeholders::_1);
		m_loop->runInLoop(std::bind(&KcpConnection::setKcpCloseCallback, kcpConn, cb));
		// ���Գ��У�ֱ��ǿ�ƹر�
		if (uniqueTcp)
		{
			tcpConn->forceClose();
		}
	}
	// kcp�����Ѿ����ͷ��ˣ���ô���ֵģ�
	else
	{
		LOG_INFO << "m_kcpConnector release!!!";
	}
}

void KcpWithTcpClient::connectTcp()
{
	// FIXME: check state
	LOG_INFO << "KcpWithTcpClient::connect[" << m_name << "] - connecting to "
			<< m_tcpConnector->serverAddress().toIpPort();

	m_tcpConnect = true;
	m_tcpConnector->start();
}

void KcpWithTcpClient::disconnectTcp()
{
	m_tcpConnect = false;

	{
		MutexLockGuard lock(m_mutex);
		if (m_tcpConnection)
		{
			m_tcpConnection->forceClose();
		}
	}
}

void KcpWithTcpClient::stopTcp()
{
	m_tcpConnect = false;
	m_tcpConnector->stop();
}

void KcpWithTcpClient::newTcpConnection(sockets::sz_sock sockfd)
{
	m_loop->assertInLoopThread();
	InetAddress peerAddr(sockets::sz_sock_getpeeraddr(sockfd));
	char buf[32];
	uint32_t connId = m_nextConnId++;
	snprintf(buf, sizeof(buf), ":tcp-%s#%d", peerAddr.toIpPort().c_str(), connId);
	string connName = m_name + buf;

	InetAddress localAddr(sockets::sz_sock_getlocaladdr(sockfd));
	// FIXME poll with zero timeout to double confirm the new connection
	// FIXME use make_shared if necessary
	TcpConnectionPtr conn(new TcpConnection(m_loop, connName, connId, sockfd, localAddr, peerAddr));
	conn->setMessageCallback(std::bind(&LengthHeaderCodec<>::onMessage, &m_tcpCodec, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	// FIXME: unsafe
	conn->setCloseCallback(std::bind(&KcpWithTcpClient::removeTcpConnection, this, std::placeholders::_1));
	// ��������ͬʱ��д���race condition
	{
		MutexLockGuard lock(m_mutex);
		m_tcpConnection = conn;
	}
	conn->connectEstablished();
}

void KcpWithTcpClient::removeTcpConnection(const TcpConnectionPtr& tcpConn)
{
	m_loop->assertInLoopThread();
	assert(m_loop == tcpConn->getLoop());
	{
		MutexLockGuard lock(m_mutex);
		assert(m_tcpConnection == tcpConn);
		m_tcpConnection.reset();
		// �����Ͽ�TCP���ӣ�����϶����ڣ���Ҫ�����ر�
		// ���Ͽ�KCP���ӣ�����϶���������
		if (m_kcpConnection)
		{
			m_kcpConnection->forceClose();
		}
		m_kcpConnection.reset();
	}

	m_loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, tcpConn));
	if (m_tcpRetry && m_tcpConnect)
	{
		LOG_INFO << "KcpWithTcpClient::connect[" << m_name << "] - Reconnecting to "
				 << m_tcpConnector->serverAddress().toIpPort();
		m_tcpConnector->restart();
	}	
}

void KcpWithTcpClient::removeKcpConnection(const KcpConnectionPtr& kcpConn)
{
	// ���ڵĻ��϶���KCP�����Ͽ����ӣ�������Ҫ�ڹر�TCP������
	if (m_tcpConnection)
	{
		MutexLockGuard lock(m_mutex);
		m_tcpConnection->forceClose();
	}
}

void KcpWithTcpClient::handleTcpMessage(const TcpConnectionPtr& tcpConn, const string& message, Timestamp receiveTime)
{
	m_loop->assertInLoopThread();
	if (message.size() != (sizeof(uint16_t) + sizeof(int32_t) + sizeof(int32_t)))
	{
		LOG_ERROR << "KcpWithTcpClient::handleTcpMessage recv size error - " << message.size();
		return;
	}
	Buffer recvBuf;
	recvBuf.append(reinterpret_cast<const void*>(message.data()), message.size());
	// ����������Ϣ
	uint16_t udpListenPort = static_cast<uint16_t>(recvBuf.readInt16());
	uint32_t conv = static_cast<uint32_t>(recvBuf.readInt32());
	int secretId = recvBuf.readInt32();
	m_udpListenAddr.resetPort(udpListenPort);
	// ����KcpConnection
	char buf[256];
	uint32_t connId = m_nextConnId++;
	snprintf(buf, sizeof(buf), ":kcp-%s#%d", m_udpListenAddr.toIpPort().c_str(), tcpConn->id());
	string connName = m_name + buf;
	m_kcpConnection.reset(new KcpConnection(m_loop, sockets::sz_udp_create(m_udpListenAddr.family()), secretId, connName, conv, true, m_kcpMode));
	m_kcpConnection->setState(KcpConnection::StateE::kConnecting);
	m_kcpConnection->setPeerAddr(m_udpListenAddr);
	m_kcpConnection->newKCP();
	m_kcpConnection->setKcpConnectionCallback(m_kcpConnectionCallback);
	m_kcpConnection->setKcpMessageCallback(m_kcpMessageCallback);
	m_kcpConnection->setKcpWriteCompleteCallback(m_kcpWriteCompleteCallback);
	m_kcpConnection->setKcpCloseCallback(std::bind(&KcpWithTcpClient::removeKcpConnection, this, std::placeholders::_1));
	m_kcpConnection->udpSendHandShakeVerify();
	m_kcpConnection->udpSendHandShakeTimer();
}

} // end namespace net

} // end namespace sznet