// Comment: IO复用基类实现

#ifndef _SZNET_NET_POLLER_H_
#define _SZNET_NET_POLLER_H_

#include "../base/NonCopyable.h"
#include "SocketsOps.h"
#include "EventLoop.h"

#include <map>
#include <vector>

namespace sznet
{

namespace net
{

// 事件分发
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
	virtual void poll(ChannelList* activeChannels, int timeoutMs = 0) = 0;
	// 更新fd的注册事件
	virtual void updateChannel(Channel* channel) = 0;
	// 删除channel
	virtual void removeChannel(Channel* channel) = 0;
	// 是否有channel
	virtual bool hasChannel(Channel* channel) const;
	// 创建IO多路复用对象
	static Poller* newDefaultPoller(EventLoop* loop);
	// 断言运行loop函数的线程必须是拥有EventLoop对象的线程
	void assertInLoopThread() const
	{
		m_ownerLoop->assertInLoopThread();
	}

protected:
	// 记录fd到Channel的对应关系 
	// 底层的多路复用API每次检查完成fd，要根据这个映射关系去寻找对应的Channel
	typedef std::map<sockets::sz_fd, Channel*> ChannelMap;
	// 保存epoll监听的fd,及其对应的Channel指针
	ChannelMap m_channels;

private:
	// 这个Poller对象所属的EventLoop
	EventLoop* m_ownerLoop;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_POLLER_H_
