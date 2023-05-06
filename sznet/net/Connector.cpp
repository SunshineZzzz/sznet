#include "Connector.h"
#include "Channel.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include "../log/Logging.h"

#include <algorithm>

namespace sznet
{

namespace net
{

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr): 
	m_loop(loop),
	m_serverAddr(serverAddr),
	m_connect(false),
	m_state(kDisconnected),
	m_retryDelayMs(kInitRetryDelayMs)
{
	LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector()
{
	LOG_DEBUG << "dtor[" << this << "]";
	assert(!m_channel);
}

void Connector::start()
{
	m_connect = true;
	// FIXME: unsafe
	m_loop->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop()
{
	m_loop->assertInLoopThread();
	assert(m_state == kDisconnected);
	if (m_connect)
	{
		connect();
	}
	else
	{
		LOG_DEBUG << "do not connect";
	}
}

void Connector::stop()
{
	m_connect = false;
	// FIXME: unsafe
	// FIXME: cancel timer
	m_loop->queueInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::stopInLoop()
{
	m_loop->assertInLoopThread();
	if (m_state == kConnecting)
	{
		setState(kDisconnected);
		sockets::sz_sock sockfd = removeAndResetChannel();
		retry(sockfd);
	}
}

void Connector::connect()
{
	sockets::sz_sock sockfd = sockets::sz_sock_createnonblockingordie(m_serverAddr.family());
	int ret = sockets::sz_sock_connect(sockfd, m_serverAddr.getSockAddr());
	int savedErrno = (ret == 0) ? 0 : sz_getlasterr();
	switch (savedErrno)
	{
	// 连接成功
	case 0:
	case sz_err_inprogress:
	// 这里不对吧？
	case sz_err_eintr:
	case sz_err_eisconn:
#ifdef SZ_OS_WINDOWS
	case sz_err_wouldblock:
#endif
		connecting(sockfd);
		break;
#ifdef SZ_OS_LINUX
	case sz_err_again:
#endif
	case sz_err_eaddrinuse:
	case sz_err_eaddrnotavail:
	case sz_err_econnrefused:
	case sz_err_enetunreach:
		retry(sockfd);
		break;
	case sz_err_eacces:
#ifdef SZ_OS_LINUX
	case sz_err_eperm:
#endif
	case sz_err_eafnosupport:
	case sz_err_ealready:
	case sz_err_ebadf:
	case sz_err_efault:
	case sz_err_enotsock:
		LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
		sockets::sz_closesocket(sockfd);
		break;
	default:
		LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
		sockets::sz_closesocket(sockfd);
		break;
	}
}

void Connector::restart()
{
	m_loop->assertInLoopThread();
	setState(kDisconnected);
	m_retryDelayMs = kInitRetryDelayMs;
	m_connect = true;
	startInLoop();
}

void Connector::connecting(sockets::sz_sock sockfd)
{
	setState(kConnecting);
	assert(!m_channel);
	m_channel.reset(new Channel(m_loop, sockfd));
	// FIXME: unsafe
	m_channel->setWriteCallback(std::bind(&Connector::handleWrite, this));
	// FIXME: unsafe
	m_channel->setErrorCallback(std::bind(&Connector::handleError, this));
	// 一会用来判断是否连接成功
	// m_channel->tie(shared_from_this()); is not working,
	// as m_channel is not managed by shared_ptr
	m_channel->enableWriting();
}

sockets::sz_sock Connector::removeAndResetChannel()
{
	m_channel->disableAll();
	m_channel->remove();
	sockets::sz_sock sockfd = m_channel->fd();
	// Can't reset m_channel here, because we are inside Channel::handleEvent
	// FIXME: unsafe
	m_loop->queueInLoop(std::bind(&Connector::resetChannel, this));
	return sockfd;
}

void Connector::resetChannel()
{
	m_channel.reset();
}

void Connector::handleWrite()
{
	LOG_TRACE << "Connector::handleWrite " << m_state;

	if (m_state == kConnecting)
	{
		sockets::sz_sock sockfd = removeAndResetChannel();
		int err = sockets::sz_sock_geterror(sockfd);
		if (err)
		{
			LOG_WARN << "Connector::handleWrite - SO_ERROR = "
				<< err << " " << sz_strerror_tl(err);
			retry(sockfd);
		}
		else if (sockets::sz_sock_isselfconnect(sockfd))
		{
			// 出现自连接，即(source IP, source port) = (destination IP, destination port)
			// 解决方式就是尝试重新连接
			LOG_WARN << "Connector::handleWrite - Self connect";
			retry(sockfd);
		}
		else
		{
			// 连接成功
			setState(kConnected);
			if (m_connect)
			{
				m_newConnectionCallback(sockfd);
			}
			else
			{
				sockets::sz_closesocket(sockfd);
			}
		}
	}
	else
	{
		// what happened?
		assert(m_state == kDisconnected);
	}
}

void Connector::handleError()
{
	LOG_ERROR << "Connector::handleError state=" << m_state;
	if (m_state == kConnecting)
	{
		sockets::sz_sock sockfd = removeAndResetChannel();
		int err = sockets::sz_sock_geterror(sockfd);
		LOG_TRACE << "SO_ERROR = " << err << " " << sz_strerror_tl(err);
		retry(sockfd);
	}
}

void Connector::retry(sockets::sz_sock sockfd)
{
	sockets::sz_closesocket(sockfd);
	setState(kDisconnected);
	if (m_connect)
	{
		LOG_INFO << "Connector::retry - Retry connecting to " << m_serverAddr.toIpPort()
			<< " in " << m_retryDelayMs << " milliseconds. ";
		m_loop->runAfter(m_retryDelayMs / 1000.0, std::bind(&Connector::startInLoop, shared_from_this()));
		// 重连延迟加倍，但不超过最大延迟
		m_retryDelayMs = std::min(m_retryDelayMs * 2, kMaxRetryDelayMs);
	}
	else
	{
		LOG_DEBUG << "do not connect";
	}
}

} // end namespace net

} // end namespace sznet
