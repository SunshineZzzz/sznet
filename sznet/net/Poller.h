#ifndef _SZNET_NET_POLLER_H_
#define _SZNET_NET_POLLER_H_

#include <map>
#include <vector>

#include "../base/NonCopyable.h"
#include "../time/Timestamp.h"
#include "EventLoop.h"

namespace sznet
{

namespace net
{

class Channel;

// IO复用类的基类
class Poller : NonCopyable
{
public:
	// 就绪事件容器类型
	typedef std::vector<Channel*> ChannelList;

	Poller(EventLoop* loop);
	virtual ~Poller();

	// Poller的核心功能，将就绪事件加入到 activeChannels 中
	virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
	// 更新fd的注册事件
	// Channel::update()->EventLoop::updateChannel(Channel* channel)->Poller::updateChannel(Channel* channel)
	virtual void updateChannel(Channel* channel) = 0;
	// 
	// EventLoop::removeChannel(Channel*)->Poller::removeChannel(Channel*)
	virtual void removeChannel(Channel* channel) = 0;
	// 
	// virtual bool hasChannel(Channel* channel) const;
	// 
	static Poller* newDefaultPoller(EventLoop* loop);
	// 
	// void assertInLoopThread() const
	// {
	// m_ownerLoop->assertInLoopThread();
	// }

protected:
	// 记录fd到Channel的对应关系 
	// 底层的多路复用API每次检查完成fd，要根据这个映射关系去寻找对应的Channel
	typedef std::map<int, Channel*> ChannelMap;
	// 保存epoll监听的fd,及其对应的Channel指针
	ChannelMap m_channels;

private:
	// 这个Poller对象所属的EventLoop
	EventLoop* m_ownerLoop;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_POLLER_H_
