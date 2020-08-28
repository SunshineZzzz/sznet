#ifndef _SZNET_NET_SELECTPOLLER_H_
#define _SZNET_NET_SELECTPOLLER_H_

#include "Channel.h"
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
	// (add/mod)更新channel中的事件
	virtual void updateChannel(Channel* channel);
	// 
	virtual void removeChannel(Channel* channel);

private:
	// 将网络IO事件对应的channel放入activeChannels中
	void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

	// 存储事件的类型
	typedef std::vector<Channel::Event_t> PollFdList;
	// 
	PollFdList m_pollfds;
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


