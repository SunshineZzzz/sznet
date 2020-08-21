#include "SelectPoller.h"
#include "Channel.h"
#include "../log/Logging.h"

#include <assert.h>
#include <errno.h>

namespace sznet
{
	
namespace net
{

SelectPoller::SelectPoller(EventLoop* loop):
	Poller(loop)
{}

SelectPoller::~SelectPoller()
{}

Timestamp SelectPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
	return Timestamp::now();
}

void SelectPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
}

void SelectPoller::updateChannel(Channel* channel)
{
}

void SelectPoller::removeChannel(Channel* channel)
{
}

} // end namespace net

} // end namespace sznet