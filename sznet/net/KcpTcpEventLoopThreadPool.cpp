#include "KcpTcpEventLoopThreadPool.h"
#include "KcpTcpEventLoop.h"
#include "KcpTcpEventLoopThread.h"

#include <random>
#include <stdio.h>

namespace sznet
{

namespace net
{

KcpTcpEventLoopThreadPool::KcpTcpEventLoopThreadPool(KcpTcpEventLoop* baseLoop, const std::vector<InetAddress>& udpListenAddrs, const string& nameArg) :
	m_baseLoop(baseLoop),
	m_name(nameArg),
	m_started(false),
	m_numThreads(0),
	m_next(0),
	m_udpListenAddrs(udpListenAddrs)
{
}

KcpTcpEventLoopThreadPool::~KcpTcpEventLoopThreadPool()
{
	// Don't delete m_baseLoop, it's stack variable
}

void KcpTcpEventLoopThreadPool::start(const ThreadInitCallback& cb)
{
	assert(!m_started);
	assert(m_numThreads == m_udpListenAddrs.size()),
		m_baseLoop->assertInLoopThread();
	m_started = true;
	std::random_device rd;
	int initSeed = rd();

	std::vector<char> vbuf(m_name.size() + 32);
	for (int i = 0; i < m_numThreads; ++i)
	{
		snprintf(vbuf.data(), vbuf.size(), "%s%d", m_name.c_str(), i);
		InetAddress addr = m_udpListenAddrs[i];
		initSeed = initSeed + i;
		KcpTcpEventLoopThread* t = new KcpTcpEventLoopThread(addr, cb, vbuf.data(), initSeed);
		m_threads.push_back(std::unique_ptr<KcpTcpEventLoopThread>(t));
		// 阻塞
		// 启动EventLoopThread线程，在进入事件循环之前，会调用cb
		m_loops.push_back(t->startLoop());
	}
	// 只有一个EventLoop，在这个EventLoop进入事件循环之前，调用cb
	if (m_numThreads == 0 && cb)
	{
		cb(m_baseLoop);
	}
}

KcpTcpEventLoop* KcpTcpEventLoopThreadPool::getNextLoop()
{
	m_baseLoop->assertInLoopThread();
	assert(m_started);
	KcpTcpEventLoop* loop = m_baseLoop;

	if (!m_loops.empty())
	{
		// round-robin
		loop = m_loops[m_next];
		++m_next;
		if (implicit_cast<size_t>(m_next) >= m_loops.size())
		{
			m_next = 0;
		}
	}
	return loop;
}

KcpTcpEventLoop* KcpTcpEventLoopThreadPool::getLoopForHash(size_t hashCode)
{
	m_baseLoop->assertInLoopThread();
	KcpTcpEventLoop* loop = m_baseLoop;

	if (!m_loops.empty())
	{
		loop = m_loops[hashCode % m_loops.size()];
	}
	return loop;
}

std::vector<KcpTcpEventLoop*> KcpTcpEventLoopThreadPool::getAllLoops()
{
	m_baseLoop->assertInLoopThread();
	assert(m_started);
	if (m_loops.empty())
	{
		return std::vector<KcpTcpEventLoop*>(1, m_baseLoop);
	}
	else
	{
		return m_loops;
	}
}

} // end namespace net

} // end namespace sznet