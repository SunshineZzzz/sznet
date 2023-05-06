#include "TcpClient.h"
#include "Connector.h"
#include "EventLoop.h"
#include "SocketsOps.h"
#include "../log/Logging.h"

#include <stdio.h>

namespace sznet
{ 

namespace net
{

namespace detail
{

// tcp�����ڶϿ����ӵĻص�����
void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
{
  loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

// ֹͣ�������ӣ�1���Ļص�����
void removeConnector(const ConnectorPtr& connector)
{
	(void)connector;
}

} // namespace detail

// TcpClient::TcpClient(EventLoop* loop)
//   : loop_(loop)
// {
// }

// TcpClient::TcpClient(EventLoop* loop, const string& host, uint16_t port)
//   : loop_(CHECK_NOTNULL(loop)),
//     serverAddr_(host, port)
// {
// }

TcpClient::TcpClient(EventLoop* loop, const InetAddress& serverAddr, const string& nameArg): 
	m_loop(CHECK_NOTNULL(loop)),
    m_connector(new Connector(loop, serverAddr)),
    m_name(nameArg),
    m_connectionCallback(defaultConnectionCallback),
    m_messageCallback(defaultMessageCallback),
    m_retry(false),
    m_connect(true),
    m_nextConnId(1)
{
	m_connector->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
	// FIXME setConnectFailedCallback
	LOG_INFO << "TcpClient::TcpClient[" << m_name 
		<< "] - connector " << get_pointer(m_connector);
}

TcpClient::~TcpClient()
{
	LOG_INFO << "TcpClient::~TcpClient[" << m_name 
		<< "] - connector " << get_pointer(m_connector);
	TcpConnectionPtr conn;
	// TCP�����Ƿ�TcpClient(owner)���Գ���
	bool unique = false;
	{
		MutexLockGuard lock(m_mutex);
		unique = m_connection.use_count() == 1;
		// TcpClient����Ҫ�����ˣ��ڲ���Ҫ������
		conn = m_connection;
	}
	if (conn)
	{
		assert(m_loop == conn->getLoop());
		// ���������Ƿ���Գ���tcp���ӣ�����Ҫ���ƺ����ˣ���ֹtcp�����ڴ�й¶
		// FIXME: not 100% safe, if we are in different thread
		CloseCallback cb = std::bind(&detail::removeConnection, m_loop, std::placeholders::_1);
		m_loop->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
		// ���Գ��У�ֱ��ǿ�ƹر�
		if (unique)
		{
			conn->forceClose();
		}
	}
	// tcp�����Ѿ����ͷ���
	else
	{
		m_connector->stop();
		// FIXME: HACK
		m_loop->runAfter(1, std::bind(&detail::removeConnector, m_connector));
	}
}

void TcpClient::connect()
{
	// FIXME: check state
	LOG_INFO << "TcpClient::connect[" << m_name << "] - connecting to "
			<< m_connector->serverAddress().toIpPort();
	m_connect = true;
	m_connector->start();
}

void TcpClient::disconnect()
{
	m_connect = false;

	{
		MutexLockGuard lock(m_mutex);
		if (m_connection)
		{
			// m_connection->shutdown();
			m_connection->forceClose();
		}
	}
}

void TcpClient::stop()
{
	m_connect = false;
	m_connector->stop();
}

void TcpClient::newConnection(sockets::sz_sock sockfd)
{
	m_loop->assertInLoopThread();
	InetAddress peerAddr(sockets::sz_sock_getpeeraddr(sockfd));
	char buf[32];
	snprintf(buf, sizeof(buf), ":%s#%d", peerAddr.toIpPort().c_str(), m_nextConnId);
	++m_nextConnId;
	string connName = m_name + buf;

	InetAddress localAddr(sockets::sz_sock_getlocaladdr(sockfd));
	// FIXME poll with zero timeout to double confirm the new connection
	// FIXME use make_shared if necessary
	TcpConnectionPtr conn(new TcpConnection(m_loop, connName, sockfd, localAddr, peerAddr));

	conn->setConnectionCallback(m_connectionCallback);
	conn->setMessageCallback(m_messageCallback);
	conn->setWriteCompleteCallback(m_writeCompleteCallback);
	// FIXME: unsafe
	conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, std::placeholders::_1));
	// ��������ͬʱ��д���race condition
	{
		MutexLockGuard lock(m_mutex);
		m_connection = conn;
	}
	conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
	m_loop->assertInLoopThread();
	assert(m_loop == conn->getLoop());

	{
		MutexLockGuard lock(m_mutex);
		assert(m_connection == conn);
		m_connection.reset();
	}

	m_loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
	if (m_retry && m_connect)
	{
		LOG_INFO << "TcpClient::connect[" << m_name << "] - Reconnecting to "
				 << m_connector->serverAddress().toIpPort();
		m_connector->restart();
	}
}

} // end namespace net

} // end namespace sznet