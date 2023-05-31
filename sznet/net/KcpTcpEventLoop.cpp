#include "KcpTcpEventLoop.h"
#include "../../3rd/kcp/ikcp.h"
#include "../log/Logging.h"
#include "../time/Timestamp.h"
#include "UdpOps.h"
#include "TcpConnection.h"
#include "KcpConnection.h"

namespace sznet
{

namespace net
{

KcpTcpEventLoop::KcpTcpEventLoop(const InetAddress udpLisetenAddr, string name, int seed) :
	EventLoop(),
	m_udpLisetenAddr(udpLisetenAddr),
	m_udpSocket(sockets::sz_udp_createnonblockingordie(udpLisetenAddr.family())),
	m_udpChannel(this, m_udpSocket.fd()),
	m_calcNetLastTime(Timestamp::now()),
	m_random(seed),
	m_curConv(0),
	m_name(name)
{
	m_udpSocket.bindAddress(m_udpLisetenAddr);
	m_udpChannel.enableReading();
	m_udpChannel.setReadCallback(std::bind(&KcpTcpEventLoop::handleUdpRead, this, std::placeholders::_1));
}

KcpTcpEventLoop::~KcpTcpEventLoop()
{
	assertVessels();
	m_udpChannel.disableAll();
	m_udpChannel.remove();
}


void KcpTcpEventLoop::removeTcpConnectionInLoop(const TcpConnectionPtr& tcpConn)
{
	LOG_INFO << "KcpTcpEventLoop::removeConnectionInLoop [" << m_name
		<< "] - connection " << tcpConn->name() << " - " << tcpConn->id();

	assertInLoopThread();
	uint32_t conv = tcpConn->id();
	TcpConnectionMap::iterator fTcpIter = m_tcpConnections.find(conv);
	assert(fTcpIter != m_tcpConnections.end());
	assert(fTcpIter->second.get() == tcpConn.get());
	m_tcpConnections.erase(conv);
	KcpConnectionMap::iterator fKcpIter = m_kcpConnections.find(conv);
	if (fKcpIter == m_kcpConnections.end())
	{
		// 说明这个时候客户端还没有回执校验就断开了 或者 KCP主动断掉连接了
		// 有可能有，有可能没有，都删除一下
		m_crypts.erase(conv);
		assertVessels();
		queueInLoop(std::bind(&TcpConnection::connectDestroyed, tcpConn));
	}
	else
	{
		KcpConnectionPtr kcpConn = fKcpIter->second;

		LOG_INFO << "KcpTcpEventLoop::removeConnectionInLoop [" << m_name
			<< "] - connection " << kcpConn->name() << " - " << kcpConn->id() << 
			"- " << kcpConn->connected();

		// kcp conn 存在，但是也有可能没有连接成功
		m_crypts.erase(conv);
		m_addrConns.erase(*kcpConn->peerAddress().getSockAddrIn());
		// 这里不处理 m_kcpConnections
		queueInLoop(std::bind(&TcpConnection::connectDestroyed, tcpConn));
		runInLoop(std::bind(&KcpConnection::forceCloseInLoop, kcpConn));
	}
}

void KcpTcpEventLoop::removeKcpConnectionInLoop(const KcpConnectionPtr& kcpConn, const int oldState)
{
	LOG_INFO << "KcpTcpEventLoop::removeConnectionInLoop [" << m_name
		<< "] - connection " << kcpConn->name() << " - " << kcpConn->id() <<
		" - " << (oldState == KcpConnection::StateE::kConnected);

	assertInLoopThread();
	uint32_t conv = kcpConn->id();
	KcpConnectionMap::iterator fKcpIter = m_kcpConnections.find(conv);
	assert(fKcpIter != m_kcpConnections.end());
	assert(fKcpIter->second.get() == kcpConn.get());
	m_kcpConnections.erase(conv);
	TcpConnectionMap::iterator fTcpIter = m_tcpConnections.find(conv);
	if (fTcpIter == m_tcpConnections.end())
	{
		// Tcp连接已经断掉了，从而走到这里
		assertVessels();
	}
	else
	{
		// Kcp连接主动断开，走到这里
		TcpConnectionPtr tcpConn = fTcpIter->second;

		LOG_INFO << "KcpTcpEventLoop::removeKcpConnectionInLoop [" << m_name
			<< "] - connection " << tcpConn->name() << " - " << tcpConn->id() <<
			"- " << tcpConn->connected();

		m_crypts.erase(conv);
		m_addrConns.erase(*kcpConn->peerAddress().getSockAddrIn());
		queueInLoop(std::bind(&KcpConnection::connectDestroyed, kcpConn));
		runInLoop(std::bind(&TcpConnection::forceCloseInLoop, tcpConn));
	}
}

void KcpTcpEventLoop::handleUdpRead(Timestamp receiveTime)
{
	assertInLoopThread();
	int savedErrno = 0;
	sz_ssize_t n = 0;
	do
	{
		n = m_inputBuffer.recvFrom(m_udpSocket.fd(), sockets::sz_sockaddr_cast(&m_curRecvAddr), &savedErrno);
	} while (n == -1 && savedErrno == sz_err_eintr);
	if (n >= 0)
	{
		m_netStatInfo.m_recvBytes += n;
	}
	else
	{
		if (!sockets::sz_wouldblock())
		{
			LOG_SYSERR << "KcpTcpEventLoop::handleUdpRead";
			return;
		}
	}

	if (m_inputBuffer.readableBytes() == 0)
	{
		LOG_ERROR << "m_inputBuffer.recvFrom return zero";
		return;
	}
	// 需要考虑客户端与服务器异步原因造成，回执结果丢包 | 客户端收到前超时再次发送握手校验
	auto fAddrConnIter = m_addrConns.find(m_curRecvAddr);
	if (fAddrConnIter == m_addrConns.end() || m_inputBuffer.readableBytes() < KcpConnection::kKCP_OVERHEAD_SIZE)
	{
		handleNewUdpMessage();
		m_inputBuffer.retrieveAll();
		return;
	}
	m_curConv = fAddrConnIter->second->getConvId();
	KcpConnectionMap::iterator fKcpIter = m_kcpConnections.find(m_curConv);
	TcpConnectionMap::iterator fTcpIter = m_tcpConnections.find(m_curConv);
	assert(fKcpIter != m_kcpConnections.end());
	assert(fTcpIter != m_tcpConnections.end());
	assert(fKcpIter->first == fTcpIter->first);
	assert(fTcpIter->second->connected());
	m_curTcpConn = fTcpIter->second;
	m_curKcpConn = fKcpIter->second;
	handleKcpMessage();
	m_inputBuffer.retrieveAll();
}

void KcpTcpEventLoop::handleNewUdpMessage()
{
	assertInLoopThread();
	size_t recvSize = m_inputBuffer.readableBytes();
	if (recvSize != (sizeof(uint32_t) + sizeof(int32_t)))
	{
		// 这种情况不予理会
		LOG_ERROR << "who are you! from " << InetAddress(m_curRecvAddr).toIpPort();
		return;
	}
	uint32_t conv = static_cast<uint32_t>(m_inputBuffer.readInt32());
	int32_t secretId = m_inputBuffer.readInt32();
	bool ok = true;
	int inError = 0;
	TcpConnectionMap::iterator fTcpIter = m_tcpConnections.find(conv);
	cryptConvMap::iterator fSecretIter = m_crypts.find(conv);
	do
	{
		if (fTcpIter == m_tcpConnections.end())
		{
			ok = false;
			inError = 1;
			break;
		}

		if (!fTcpIter->second->connected())
		{
			ok = false;
			inError = 2;
			break;
		}

		if (fSecretIter == m_crypts.end())
		{
			ok = false;
			inError = 3;
			break;
		}

		if (fSecretIter->second != secretId)
		{
			ok = false;
			inError = 4;
			break;
		}

		inError = 0;
		ok = true;
	} while (false);
	if (!ok)
	{
		LOG_ERROR << "you are hack! [" << inError << "] from " << InetAddress(m_curRecvAddr).toIpPort();
		// 回执客户端
		checkResponse(ok);
		return;
	}
	KcpConnectionMap::iterator fKcpIter = m_kcpConnections.find(conv);
	if (fKcpIter == m_kcpConnections.end())
	{
		// 创建KcpConnection
		char buf[256];
		snprintf(buf, sizeof(buf), "-kcp-%s#%d", getUdpListenIpPort().c_str(), conv);
		string connName = m_name + buf;
		KcpConnectionPtr kcpConn(new KcpConnection(this, m_udpSocket.fd(), secretId, connName, conv, false));
		kcpConn->setState(KcpConnection::StateE::kConnecting);
		kcpConn->setPeerAddr(InetAddress(m_curRecvAddr));
		kcpConn->newKCP();
		m_kcpConnections[conv] = kcpConn;
		kcpConn->setKcpConnectionCallback(m_kcpConnectionCallback);
		kcpConn->setKcpMessageCallback(m_kcpMessageCallback);
		kcpConn->setKcpWriteCompleteCallback(m_kcpWriteCompleteCallback);
		kcpConn->setKcpCloseCallback(std::bind(&KcpTcpEventLoop::removeKcpConnectionInLoop, this, std::placeholders::_1, std::placeholders::_2));
		// 映射
		m_addrConns[m_curRecvAddr] = kcpConn;
	}
	checkResponse(ok);
}

void KcpTcpEventLoop::handleKcpMessage()
{
	assertInLoopThread();
	assert(m_curKcpConn);
	assert(m_curTcpConn);
	if (m_curKcpConn->connecting())
	{
		// 说明第一哦
		m_curKcpConn->connectEstablished();
	}
	m_curKcpConn->handleKcpMessage(m_inputBuffer);
}

void KcpTcpEventLoop::checkResponse(bool isSuccess)
{
	int data = isSuccess ? 1 : 0;
	sockets::sz_udp_send(m_udpSocket.fd(), reinterpret_cast<const char*>(&data), sizeof(int), InetAddress(m_curRecvAddr).getSockAddr());
}

} // end namespace net

} // end namespace sznet