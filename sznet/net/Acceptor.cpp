#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"
#include "Channel.h"
#include "../io/IO.h"
#include "../log/Logging.h"

#include <errno.h>
#include <fcntl.h>

// 创建空洞文件
int createNullFileFd()
{
#if defined(SZ_OS_WINDOWS)
	return sznet::sz_open("nul", O_RDONLY);
#else
	return sznet::sz_open("/dev/null", O_RDONLY | O_CLOEXEC);
#endif
}

namespace sznet
{

namespace net
{

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport): 
	m_loop(loop),
	m_acceptSocket(sockets::sz_sock_createnonblockingordie(listenAddr.family())),
	m_acceptChannel(loop, m_acceptSocket.fd()),
	m_listenning(false),
	m_idleFd(createNullFileFd())
{
	assert(m_idleFd >= 0);
	m_acceptSocket.setReuseAddr(true);
	m_acceptSocket.setReusePort(reuseport);
	m_acceptSocket.bindAddress(listenAddr);
	m_acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
	m_acceptChannel.disableAll();
	m_acceptChannel.remove();
	sockets::sz_closesocket(m_idleFd);
}

void Acceptor::listen()
{
	m_loop->assertInLoopThread();
	m_listenning = true;
	m_acceptSocket.listen();
	m_acceptChannel.enableReading();
}

void Acceptor::handleRead()
{
	m_loop->assertInLoopThread();
	InetAddress peerAddr;
	// FIXME loop until no more
	sockets::sz_sock connfd = m_acceptSocket.accept(&peerAddr);
	if (connfd >= 0)
	{
		// string hostport = peerAddr.toIpPort();
		// LOG_TRACE << "Accepts of " << hostport;
		if (m_newConnectionCallback)
		{
			m_newConnectionCallback(connfd, peerAddr);
		}
		else
		{
			// 没有回调函数则关闭client对应的fd
			sockets::sz_closesocket(connfd);
		}
	}
	// 接收套接字失败
	else
	{
		LOG_SYSERR << "in Acceptor::handleRead";
		// 返回的错误码为EMFILE，超过最大连接数
		if (sz_getlasterr() == sz_err_emfile)
		{
			// 防止busy loop
			sockets::sz_closesocket(m_idleFd);
			m_idleFd = ::accept(m_acceptSocket.fd(), nullptr, nullptr);
			sockets::sz_closesocket(m_idleFd);
			m_idleFd = createNullFileFd();
		}
	}
}

} // end namespace net

} // end namespace sznet