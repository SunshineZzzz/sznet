#include "Poller.h"
#include "Channel.h"

namespace sznet 
{

namespace net
{

Poller::Poller(EventLoop* loop): 
	m_ownerLoop(loop)
{
}

Poller::~Poller() = default;

bool Poller::hasChannel(Channel* channel) const
{
	assertInLoopThread();
	ChannelMap::const_iterator it = m_channels.find(channel->fd());
	return it != m_channels.end() && it->second == channel;
}

} // end namespace net

} // end namespace sznet 