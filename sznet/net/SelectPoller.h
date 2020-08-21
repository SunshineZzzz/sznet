#ifndef _SZNET_NET_SELECTPOLLER_H_
#define _SZNET_NET_SELECTPOLLER_H_

#include "Poller.h"
#include <vector>

#ifdef SZ_OS_WINDOWS
#	include <WinSock2.h>
#endif

#ifdef SZ_OS_LINUX
#	include <sys/select.h>
#endif

namespace sznet
{

namespace net
{

class SelectPoller :public Poller
{
public:
	SelectPoller(EventLoop* loop);
	virtual ~SelectPoller();

	// 获取网络IO事件
	virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
	// 
	virtual void updateChannel(Channel* channel);
	// 
	virtual void removeChannel(Channel* channel);

private:
	void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

	// 
	// typedef std::vector<pollfd> PollFdList;
	// 
	// PollFdList m_pollfds;
	// 可读事件集合
	fd_set m_rfds;
	// 可写事件集合
	fd_set m_wfds;
	// 错误事件集合
	fd_set m_efds;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_SELECTPOLLER_H_


