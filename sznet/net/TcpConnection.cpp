#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "SocketsOps.h"
#include "../base/WeakCallback.h"
#include "../log/Logging.h"

#include <errno.h>

namespace sznet 
{

namespace net
{

void defaultConnectionCallback(const TcpConnectionPtr& conn)
{
	LOG_TRACE << conn->localAddress().toIpPort() << " -> "
		<< conn->peerAddress().toIpPort() << " is "
		<< (conn->connected() ? "UP" : "DOWN");
	// do not call conn->forceClose(), because some users want to register message callback only.
}

void defaultMessageCallback(const TcpConnectionPtr&, Buffer* buf, Timestamp)
{
	buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop, const string& nameArg, const uint32_t id, sockets::sz_sock sockfd, const InetAddress& localAddr, const InetAddress& peerAddr): 
	m_loop(CHECK_NOTNULL(loop)),
	m_name(nameArg),
	m_id(id),
	m_state(kConnecting),
	m_reading(true),
	m_socket(new Socket(sockfd)),
	m_channel(new Channel(loop, sockfd)),
	m_localAddr(localAddr),
	m_peerAddr(peerAddr),
	m_highWaterMark(64 * 1024 * 1024),
	m_calcNetLastTime(Timestamp::now())
{
	// 设置可读
	m_channel->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
	// 设置可写
	m_channel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
	// 设置关闭
	m_channel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
	// 设置错误
	m_channel->setErrorCallback(std::bind(&TcpConnection::handleError, this));
	LOG_DEBUG << "TcpConnection::ctor[" << m_name << "] at " << this << " fd=" << sockfd;
	// 开启保活
	m_socket->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
	LOG_DEBUG << "TcpConnection::dtor[" << m_name << "] at " << this
		<< " fd=" << m_channel->fd()
		<< " state=" << stateToString();
	assert(m_state == kDisconnected);
}

void TcpConnection::send(const void* data, int len)
{
	send(StringPiece(static_cast<const char*>(data), len));
}

void TcpConnection::send(const StringPiece& message)
{
	if (m_state == kConnected)
	{
		if (m_loop->isInLoopThread())
		{
			sendInLoop(message);
		}
		else
		{
			void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
			// FIXME std::forward<string>(message)
			m_loop->runInLoop(std::bind(fp, this, message.as_string()));
		}
	}
}

// FIXME efficiency!!!
void TcpConnection::send(Buffer* buf)
{
	if (m_state == kConnected)
	{
		if (m_loop->isInLoopThread())
		{
			sendInLoop(buf->peek(), buf->readableBytes());
			buf->retrieveAll();
		}
		else
		{
			void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
			// FIXME std::forward<string>(message)
			m_loop->runInLoop(std::bind(fp, this, buf->retrieveAllAsString()));
		}
	}
}

void TcpConnection::sendInLoop(const StringPiece& message)
{
	sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
	m_loop->assertInLoopThread();
	// 已发数据
	sz_ssize_t nwrote = 0;
	// 剩余数据
	size_t remaining = len;
	// 发生错误
	bool faultError = false;

	if (m_state == kDisconnected)
	{
		LOG_WARN << "disconnected, give up writing";
		return;
	}

	// 没有设置可写事件 && 发送缓冲区没有数据，尝试主动发送数据
	if (!m_channel->isWriting() && m_outputBuffer.readableBytes() == 0)
	{
		nwrote = sockets::sz_socket_write(m_channel->fd(), data, len);
		if (nwrote >= 0)
		{
			m_netStatInfo.m_sendBytes += nwrote;
			remaining = len - nwrote;
			if (remaining == 0 && m_writeCompleteCallback)
			{
				m_loop->queueInLoop(std::bind(m_writeCompleteCallback, shared_from_this()));
			}
		}
		// nwrote < 0
		else
		{
			nwrote = 0;
			if (sz_getlasterr() != sz_err_eintr && !sockets::sz_wouldblock())
			{
				faultError = true;
			}
		}
	}
	
	// 全部发完了/发了部分/没发出去/出错了
	assert(remaining <= len);
	if (!faultError && remaining > 0)
	{
		size_t oldLen = m_outputBuffer.readableBytes();
		// 发送数据缓冲区没有超过高水位线，并且待发送数据主动发送失败，加入缓冲区后超过高水位线后进行回调
		if (oldLen + remaining >= m_highWaterMark && oldLen < m_highWaterMark && m_highWaterMarkCallback)
		{
			m_loop->queueInLoop(std::bind(m_highWaterMarkCallback, shared_from_this(), oldLen + remaining));
		}
		m_outputBuffer.append(static_cast<const char*>(data) + nwrote, remaining);
		// 注册可写事件
		if (!m_channel->isWriting())
		{
			m_channel->enableWriting();
		}
	}
}

void TcpConnection::shutdown()
{
	// FIXME: use compare and swap
	if (m_state == kConnected)
	{
		setState(kDisconnecting);
		// FIXME: shared_from_this()?
		m_loop->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
	}
}

void TcpConnection::shutdownInLoop()
{
	m_loop->assertInLoopThread();
	// 没有注册写事件的话，半关闭，关闭写端
	if (!m_channel->isWriting())
	{
		m_socket->shutdownWrite();
	}
}

void TcpConnection::forceClose()
{
	// FIXME: use compare and swap
	if (m_state == kConnected || m_state == kDisconnecting)
	{
		setState(kDisconnecting);
		m_loop->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
	}
}

void TcpConnection::forceCloseWithDelay(double seconds)
{
	if (m_state == kConnected || m_state == kDisconnecting)
	{
		setState(kDisconnecting);
		// not forceCloseInLoop to avoid race condition
		m_loop->runAfter(
			seconds,
			makeWeakCallback(shared_from_this(),
				&TcpConnection::forceClose));
	}
}

void TcpConnection::forceCloseInLoop()
{
	m_loop->assertInLoopThread();
	if (m_state == kConnected || m_state == kDisconnecting)
	{
		// as if we received 0 byte in handleRead();
		handleClose();
	}
}

const char* TcpConnection::stateToString() const
{
	switch (m_state)
	{
	case kDisconnected:
		return "kDisconnected";
	case kConnecting:
		return "kConnecting";
	case kConnected:
		return "kConnected";
	case kDisconnecting:
		return "kDisconnecting";
	default:
		return "unknown state";
	}
}

void TcpConnection::setTcpNoDelay(bool on)
{
	m_socket->setTcpNoDelay(on);
}

void TcpConnection::startRead()
{
	m_loop->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop()
{
	m_loop->assertInLoopThread();
	if (!m_reading || !m_channel->isReading())
	{
		m_channel->enableReading();
		m_reading = true;
	}
}

void TcpConnection::stopRead()
{
	m_loop->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop()
{
	m_loop->assertInLoopThread();
	if (m_reading || m_channel->isReading())
	{
		m_channel->disableReading();
		m_reading = false;
	}
}

void TcpConnection::shrinkRSBuffer(size_t sendSize, size_t recvSize)
{
	m_loop->runInLoop(std::bind(&TcpConnection::shrinkRSBufferInLoop, this, sendSize, recvSize));
}

void TcpConnection::shrinkRSBufferInLoop(size_t sendSize, size_t recvSize)
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

void TcpConnection::connectEstablished()
{
	m_loop->assertInLoopThread();
	assert(m_state == kConnecting);
	setState(kConnected);
	m_channel->tie(shared_from_this());
	m_channel->enableReading();

	if (m_connectionCallback)
	{
		m_connectionCallback(shared_from_this());
	}
}

void TcpConnection::connectDestroyed()
{
	m_loop->assertInLoopThread();
	if (m_state == kConnected)
	{
		setState(kDisconnected);
		m_channel->disableAll();

		m_connectionCallback(shared_from_this());
	}
	m_channel->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
	m_loop->assertInLoopThread();
	int savedErrno = 0;
	sz_ssize_t n = 0;
	while (true)
	{
		do
		{
			n = m_inputBuffer.readvFd(m_channel->fd(), &savedErrno);
		} while (n == -1 && savedErrno == sz_err_eintr);
		if (n > 0)
		{
			m_netStatInfo.m_recvBytes += n;
		}
		else if (n == 0)
		{
			handleClose();
			return;
		}
		else
		{
			if (!sockets::sz_wouldblock())
			{
				LOG_SYSERR << "TcpConnection::handleRead";
				handleError();
				handleClose();
			}
			// 数据全部收取完成
			break;
		}
	}
	// 处理数据
	m_messageCallback(shared_from_this(), &m_inputBuffer, receiveTime);
}

void TcpConnection::handleWrite()
{
	m_loop->assertInLoopThread();
	if (m_channel->isWriting())
	{
		sz_ssize_t n = 0;
		while(m_outputBuffer.readableBytes() > 0)
		{
			do
			{
				n = sockets::sz_socket_write(m_channel->fd(), m_outputBuffer.peek(), m_outputBuffer.readableBytes());
			} while (n == -1 && sz_getlasterr() == sz_err_eintr);
			if (n > 0)
			{
				m_netStatInfo.m_sendBytes += n;
				m_outputBuffer.retrieve(n);
				if (m_outputBuffer.readableBytes() == 0)
				{
					m_channel->disableWriting();
					if (m_writeCompleteCallback)
					{
						m_loop->queueInLoop(std::bind(m_writeCompleteCallback, shared_from_this()));
					}
					if (m_state == kDisconnecting)
					{
						shutdownInLoop();
					}
				}
			}
			else
			{
				if (!sockets::sz_wouldblock())
				{
					LOG_SYSERR << "TcpConnection::handleWrite";
					handleError();
					handleClose();
				}
				return;
			}
		}
	}
	else
	{
		LOG_TRACE << "Connection fd = " << m_channel->fd()
			<< " is down, no more writing";
	}
}

void TcpConnection::handleClose()
{
	m_loop->assertInLoopThread();
	LOG_TRACE << "fd = " << m_channel->fd() << " state = " << stateToString();
	assert(m_state == kConnected || m_state == kDisconnecting);
	// we don't close fd, leave it to dtor, so we can find leaks easily.
	setState(kDisconnected);
	m_channel->disableAll();

	// 防止提前释放，造成不可预估的情况
	TcpConnectionPtr guardThis(shared_from_this());
	if (m_connectionCallback)
	{
		m_connectionCallback(guardThis);
	}
	// must be the last line
	m_closeCallback(guardThis);
}

void TcpConnection::handleError()
{
	int err = sockets::sz_sock_geterror(m_socket->fd());
	LOG_ERROR << "TcpConnection::handleError [" << m_name
		<< "] - SO_ERROR = " << err << " " << sz_strerror_tl(err);
}

} // end namespace net

} // end namespace sznet