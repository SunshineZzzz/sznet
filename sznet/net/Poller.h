#ifndef _SZNET_NET_POLLER_H_
#define _SZNET_NET_POLLER_H_

#include "../base/NonCopyable.h"
#include "../time/Timestamp.h"
#include "SocketsOps.h"
#include "EventLoop.h"

#include <map>
#include <vector>

namespace sznet
{

namespace net
{

// �¼��ַ�
class Channel;

// IO������Ļ���
class Poller : NonCopyable
{
public:
	// �����¼���������
	typedef std::vector<Channel*> ChannelList;

	Poller(EventLoop* loop);
	virtual ~Poller();

	// Poller�ĺ��Ĺ��ܣ��������¼����뵽 activeChannels ��
	virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
	// ����fd��ע���¼�
	// Channel::update()->EventLoop::updateChannel(Channel* channel)->Poller::updateChannel(Channel* channel)
	virtual void updateChannel(Channel* channel) = 0;
	// 
	// EventLoop::removeChannel(Channel*)->Poller::removeChannel(Channel*)
	virtual void removeChannel(Channel* channel) = 0;
	// 
	// virtual bool hasChannel(Channel* channel) const;
	// 
	static Poller* newDefaultPoller(EventLoop* loop);
	// ��������loop�������̱߳�����ӵ��EventLoop������߳�
	void assertInLoopThread() const
	{
		m_ownerLoop->assertInLoopThread();
	}

protected:
	// ��¼fd��Channel�Ķ�Ӧ��ϵ 
	// �ײ�Ķ�·����APIÿ�μ�����fd��Ҫ�������ӳ���ϵȥѰ�Ҷ�Ӧ��Channel
	typedef std::map<sockets::sz_fd, Channel*> ChannelMap;
	// ����epoll������fd,�����Ӧ��Channelָ��
	ChannelMap m_channels;

private:
	// ���Poller����������EventLoop
	EventLoop* m_ownerLoop;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_POLLER_H_
