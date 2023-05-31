#include "KcpConnection.h"
#include "../log/Logging.h"
#include "../base/WeakCallback.h"
#include "../string/StringPiece.h"
#include "UdpOps.h"
#include "KcpTcpEventLoop.h"

namespace sznet
{

namespace net
{

void defaultKcpConnectionCallback(const KcpConnectionPtr& conn)
{
	LOG_TRACE << conn->localAddress().toIpPort() << " -> "
		<< conn->peerAddress().toIpPort() << " is "
		<< (conn->connected() ? "UP" : "DOWN");
	// do not call conn->forceClose(), because some users want to register message callback only.
}

void defaultKcpMessageCallback(const KcpConnectionPtr&, Buffer* buf, Timestamp)
{
	buf->retrieveAll();
}

const double KcpConnection::kEverySendVerify = 10.0;

KcpConnection::KcpConnection(EventLoop* loop, sockets::sz_sock fd, int secretId, const string& nameArg, const uint32_t id, bool isClient, const KcpSettings& kcpSet) :
	m_loop(CHECK_NOTNULL(loop)),
	m_udpSocket(fd, isClient),
	m_name(nameArg),
	m_id(id),
	m_state(kDisconnected),
	m_peerAddr(nullptr),
	m_kcpConnectionCallback(defaultKcpConnectionCallback),
	m_kcpMessageCallback(defaultKcpMessageCallback),
	m_highWaterMark(64 * 1024 * 1024),
	m_calcNetLastTime(Timestamp::now()),
	m_kcp(nullptr),
	m_isClient(isClient),
	m_secretId(secretId),
	m_kcpSet(kcpSet)
{
	m_loop->assertInLoopThread();
	LOG_DEBUG << "KcpConnection::ctor[" << m_name << "] at " << this << " id=" << id;
	if (isClient)
	{
		
		m_udpChannel.reset(new Channel(m_loop, fd));
		m_udpChannel->setReadCallback(std::bind(&KcpConnection::handleUdpMessage, this, std::placeholders::_1));
		m_udpChannel->enableReading();
	}
}

KcpConnection::~KcpConnection()
{
	LOG_DEBUG << "KcpConnection::dtor[" << m_name << "] at " << this
		<< " id=" << m_id
		<< " state=" << stateToString();
	assert(m_state == kDisconnected);
	m_loop->cancel(m_kcpUpdateTimerId);
	m_loop->cancel(m_verifyTimerId);
	if (m_kcp)
	{
		ikcp_release(m_kcp);
		m_kcp = nullptr;
	}
	if (m_udpChannel && m_isClient)
	{
		m_udpChannel->disableAll();
		m_udpChannel->remove();
	}
}

void KcpConnection::send(const void* data, int len)
{
	send(StringPiece(static_cast<const char*>(data), len));
}

void KcpConnection::send(const StringPiece& message)
{
	if (m_state == kConnected)
	{
		if (m_loop->isInLoopThread())
		{
			sendInLoop(message);
		}
		else
		{
			void (KcpConnection:: * fp)(const StringPiece & message) = &KcpConnection::sendInLoop;
			// FIXME std::forward<string>(message)
			m_loop->runInLoop(std::bind(fp, this, message.as_string()));
		}
	}
}

// FIXME efficiency!!!
void KcpConnection::send(Buffer* buf)
{
	if (m_state == kConnected)
	{
		if (m_loop->isInLoopThread())
		{
			sendInLoop(buf->peek(), static_cast<int>(buf->readableBytes()));
			buf->retrieveAll();
		}
		else
		{
			void (KcpConnection:: * fp)(const StringPiece & message) = &KcpConnection::sendInLoop;
			// FIXME std::forward<string>(message)
			m_loop->runInLoop(std::bind(fp, this, buf->retrieveAllAsString()));
		}
	}
}

void KcpConnection::sendInLoop(const StringPiece& message)
{
	sendInLoop(message.data(), message.size());
}

void KcpConnection::sendInLoop(const char* data, int len)
{
	m_loop->assertInLoopThread();

	if (m_state == kDisconnected || m_state == kConnecting)
	{
		LOG_WARN << "disconnected or kConnecting, give up writing";
		return;
	}

	// 上次逻辑层等待发送的数据大小
	int lastLogicWaitSendSize = static_cast<int>(m_outputBuffer.readableBytes());
	// kcp内部等待发送的数据大小
	int waitSendSize = ikcp_waitsnd(m_kcp);
	// 逻辑层 + 本次 + 底层等待发送数据大小
	int sumSendSize = lastLogicWaitSendSize + waitSendSize + len;
	// 超过2倍snd_wnd，那就等等把
	int thresholdSize = (m_kcpSet.m_sndWnd * m_kcpSet.m_mtu) * 2;
	if (sumSendSize >= thresholdSize)
	{
		LOG_INFO << "ikcp_waitsnd beyond threshold,[thresholdSize=" << thresholdSize 
			<< "],[waitSendSize=" << waitSendSize << "],[lastLogicWaitSendSize=" << lastLogicWaitSendSize
			<< "],[size=" << len << "]";
		if (implicit_cast<size_t>(lastLogicWaitSendSize + len) >= m_highWaterMark && implicit_cast<size_t>(lastLogicWaitSendSize) < m_highWaterMark && m_kcpHighWaterMarkCallback)
		{
			m_loop->queueInLoop(std::bind(m_kcpHighWaterMarkCallback, shared_from_this(), implicit_cast<size_t>(lastLogicWaitSendSize + len)));
		}
		m_outputBuffer.append(data, implicit_cast<size_t>(len));
		return;
	}

	int sendRet = ikcp_send(m_kcp, data, len);
	if (sendRet < 0)
	{
		LOG_FATAL << "ikcp_send return error,[sendRet=" << sendRet << "]";
		return;
	}
	ikcp_flush(m_kcp);
	kcpUpdateManual();

	m_netStatInfo.m_sendBytes += len;
	if (m_kcpWriteCompleteCallback)
	{
		m_loop->queueInLoop(std::bind(m_kcpWriteCompleteCallback, shared_from_this()));
	}
}

void KcpConnection::forceClose()
{
	// FIXME: use compare and swap
	if (m_state == kConnected || m_state == kConnecting || m_state == kDelayDisconnecting)
	{
		setState(kDisconnecting);
		m_loop->queueInLoop(std::bind(&KcpConnection::forceCloseInLoop, shared_from_this()));
	}
}

void KcpConnection::forceCloseWithDelay(double seconds)
{
	if (m_state == kConnected || m_state == kConnecting)
	{
		setState(kDelayDisconnecting);
		// not forceCloseInLoop to avoid race condition
		m_loop->runAfter(
			seconds,
			makeWeakCallback(shared_from_this(),
				&KcpConnection::forceClose));
	}
}

void KcpConnection::forceCloseInLoop()
{
	m_loop->assertInLoopThread();
	if (m_state == kConnected || m_state == kConnecting || m_state == kDisconnecting)
	{
		handleKcpClose();
	}
}

void KcpConnection::udpSendHandShakeVerify()
{
	m_loop->assertInLoopThread();
	Buffer buf;
	buf.appendInt32(m_id);
	buf.appendInt32(m_secretId);
	sockets::sz_udp_send(m_udpSocket.fd(), reinterpret_cast<const void*>(buf.peek()), static_cast<int>(buf.readableBytes()), m_peerAddr->getSockAddr());
}

void KcpConnection::udpSendHandShakeTimer()
{
	m_verifyTimerId = m_loop->runEvery(KcpConnection::kEverySendVerify, std::bind(&KcpConnection::udpSendHandShakeVerify, this));
}

int KcpConnection::kcpOutput(const char* buf, int len, ikcpcb* kcp, void* user)
{
	// 这里不包装epoll或者其他类似，只是因为UDP的EAGAIN出现可能性很低很低，除非
	// while循环往死的发送，超过了底层网卡缓存才会出现，这个和TCP的EAGAIN是两回事
	KcpConnection* pKcpConnection = static_cast<KcpConnection*>(user);
	assert(pKcpConnection->connected());
	assert(pKcpConnection->m_peerAddr);
	pKcpConnection->assertInLoopThread();
	sz_ssize_t ret = sockets::sz_udp_send(pKcpConnection->m_udpSocket.fd(), buf, len, pKcpConnection->m_peerAddr->getSockAddr());
	if (ret == -1)
	{
		LOG_ERROR << "sockets::sz_udp_send error";
	}
	return static_cast<int>(ret);
}

const char* KcpConnection::stateToString() const
{
	switch (m_state)
	{
	case kDisconnected:
		return "kDisconnected";
	case kDelayDisconnecting:
		return "kDelayDisconnecting";
	case kDisconnecting:
		return "kDisconnecting";
	case kConnecting:
		return "kConnecting";
	case kConnected:
		return "kConnected";
	default:
		return "unknown state";
	}
}

void KcpConnection::shrinkRSBufferInLoop(size_t sendSize, size_t recvSize)
{
	m_loop->assertInLoopThread();
	if (sendSize > 0)
	{
		m_outputBuffer.shrink(sendSize);
	}
	if (recvSize > 0)
	{
		m_inputBuffer.shrink(recvSize);
	}
}

void KcpConnection::kcpUpdateManual()
{
	LOG_INFO << "kcpUpdateManual";
	m_loop->assertInLoopThread();
	assert(m_state == kConnected);
	Timestamp curTimeStamp = Timestamp::now();
	uint32_t curTime = static_cast<uint32_t>(curTimeStamp.milliSecondsSinceEpoch());
	ikcp_update(m_kcp, curTime);
	uint32_t nextTime = ikcp_check(m_kcp, curTime);
	assert(nextTime >= curTime);
	Timestamp nextTimestamp = Timestamp(static_cast<int64_t>(curTimeStamp.microSecondsSinceEpoch() + (nextTime - curTime) * Timestamp::kMicroSecondsPerMillisecond));
	assert(nextTimestamp.valid());
	m_loop->cancel(m_kcpUpdateTimerId);
	m_kcpUpdateTimerId = m_loop->runAt(nextTimestamp, std::bind(&KcpConnection::kcpUpdateManual, this));
}

void KcpConnection::handleUdpMessage(Timestamp receiveTime)
{
	assert(m_isClient);
	m_loop->assertInLoopThread();
	int savedErrno = 0;
	sz_ssize_t n = 0;
	sockaddr_in recvAddr;
	static Buffer recvBuffer;
	recvBuffer.retrieveAll();
	do
	{
		n = recvBuffer.recvFrom(m_udpSocket.fd(), sockets::sz_sockaddr_cast(&recvAddr), &savedErrno);
	} while (n == -1 && savedErrno == sz_err_eintr);

	if (n < 0)
	{
		if (!sockets::sz_wouldblock())
		{
			LOG_SYSERR << "KcpConnection::handleUdpMessage";
			return;
		}
	}

	if (recvBuffer.readableBytes() == 0)
	{
		LOG_ERROR << "recvBuffer.recvFrom return zero";
		return;
	}
	
	// 这不太可能把
	if (memcmp(&recvAddr, m_peerAddr->getSockAddr(), sizeof(sockaddr_in)) != 0)
	{
		LOG_ERROR << "recvAddr not equal m_udpListenAddr";
		return;
	}

	if (recvBuffer.readableBytes() < kKCP_OVERHEAD_SIZE)
	{
		// 握手检验的结果
		assert(recvBuffer.readableBytes() == sizeof(int));
		int success = recvBuffer.readInt32();
		if (!success)
		{
			LOG_ERROR << "handshark verify failed";
			disconnected();
		}
		else
		{
			LOG_INFO << "handshark verify ok";
			if (connecting())
			{
				connectEstablished();
			}
		}
		m_loop->cancel(m_verifyTimerId);
		recvBuffer.retrieveAll();
		return;
	}
	// KCP消息咯
	handleKcpMessage(recvBuffer);
	recvBuffer.retrieveAll();
}

void KcpConnection::connectEstablished()
{
	// 对于客户端而言：客户端拿到服务器UDP的验证结果以后就算成功，会进行update timer的设置，
	// 服务器这个时候并没有开启update timer，而是再收到客户端消息的时候才会算连接成功，走到这里。
	// 这里消息可不一定是逻辑消息，大有可能是KCP内部消息！
	m_loop->assertInLoopThread();
	assert(m_state == kConnecting);
	setState(kConnected);
	m_kcpConnectionCallback(shared_from_this());
	m_loop->cancel(m_kcpUpdateTimerId);
	auto atRunTime = Timestamp::now().microSecondsSinceEpoch() + Timestamp::kMicroSecondsPerMillisecond * kFirstKcpUpdateMilliInterval;
	m_loop->runAt(Timestamp(atRunTime), std::bind(&KcpConnection::kcpUpdateManual, this));
}

void KcpConnection::shrinkRSBuffer(size_t sendSize, size_t recvSize)
{
	m_loop->runInLoop(std::bind(&KcpConnection::shrinkRSBufferInLoop, this, sendSize, recvSize));
}

void KcpConnection::newKCP()
{
	m_loop->assertInLoopThread();
	m_kcp = ikcp_create(m_id, this);
	CHECK_NOTNULL(m_kcp);
	ikcp_setoutput(m_kcp, kcpOutput);
	ikcp_nodelay(m_kcp, m_kcpSet.m_noDelay, m_kcpSet.m_interval, m_kcpSet.m_resend, m_kcpSet.m_noFlowControl);
	ikcp_wndsize(m_kcp, m_kcpSet.m_resend, m_kcpSet.m_rcvWnd);
	m_kcp->stream = m_kcpSet.m_isStream;
	m_kcp->rx_minrto = m_kcpSet.m_minrto;
}

void KcpConnection::handleKcpMessage(Buffer& inputBuffer)
{
	long readSize = static_cast<long>(inputBuffer.readableBytes());
	ikcp_input(m_kcp, inputBuffer.peek(), readSize);
	kcpUpdateManual();
	int bufferSize = static_cast<int>(m_inputBuffer.writableBytes());
	int recvRet = ikcp_recv(m_kcp, m_inputBuffer.beginWrite(), bufferSize);
	if (recvRet < 0)
	{
		// https://github.com/skywind3000/kcp/issues/39
		// -1 代表完全没有数据，可以理解成 EAGAIN
		// -2 代表数据不全（需要后续数据才能作为一个完整包），也可以理解成 EAGAIN
		// -3 代表你传入的缓存长度不够
		if (recvRet == -1 || recvRet == -2)
		{
			LOG_TRACE << "ikcp_recv EAGAIN ";
		}
		else
		{
			LOG_ERROR << "ikcp_recv recv buffer size not enough";
		}
		return;
	}
	m_inputBuffer.hasWritten(static_cast<size_t>(recvRet));
	// 处理数据
	m_kcpMessageCallback(shared_from_this(), &m_inputBuffer, Timestamp::now());
}

void KcpConnection::handleKcpClose()
{
	m_loop->assertInLoopThread();
	LOG_TRACE << "fd = " << m_udpSocket.fd() << " state = " << stateToString();
	int oldState = implicit_cast<int>(m_state);
	assert(m_state == kConnected || m_state == kConnecting || m_state == kDisconnecting);
	// we don't close fd, leave it to dtor, so we can find leaks easily.
	setState(kDisconnected);

	// 防止提前释放，造成不可预估的情况
	KcpConnectionPtr guardThis(shared_from_this());
	m_kcpConnectionCallback(guardThis);
	// must be the last line
	m_kcpCloseCallback(guardThis, oldState);
}

void KcpConnection::connectDestroyed()
{
	m_loop->assertInLoopThread();
	if (m_state == kConnected)
	{
		setState(kDisconnected);
		m_kcpConnectionCallback(shared_from_this());
	}
}

} // end namespace net

} // end namespace sznet