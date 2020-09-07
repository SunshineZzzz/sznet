#include "SelectPoller.h"
#include "../log/Logging.h"

#include <assert.h>
#include <errno.h>

namespace sznet
{
	
namespace net
{

SelectPoller::SelectPoller(EventLoop* loop):
	Poller(loop)
{
}

SelectPoller::~SelectPoller()
{
}

void SelectPoller::poll(ChannelList* activeChannels, int timeoutMs)
{
	if (m_pollfds.empty())
	{
		LOG_TRACE << "m_pollfds is empty, nothing happened";
		return;
	}

	FD_ZERO(&m_rfds);
	FD_ZERO(&m_wfds);
	FD_ZERO(&m_efds);
	sockets::sz_fd maxfd = 0;
	for (const auto& elem : m_pollfds)
	{
		if (elem.ev & Channel::kNoneEvent)
		{
			continue;
		}
		if (elem.ev & Channel::kReadEvent)
		{
			FD_SET(elem.fd, &m_rfds);
		}
		if (elem.ev & Channel::kWriteEvent)
		{
			FD_SET(elem.fd, &m_wfds);
		}
		FD_SET(elem.fd, &m_efds);
		if (elem.fd > maxfd)
		{
			maxfd = elem.fd;
		}
	}

	timeval timeout;
	timeout.tv_sec = timeoutMs / 1000;
	timeout.tv_usec = static_cast<int>(timeoutMs % 1000) * 1000;
	int numEvents = select(static_cast<int>(maxfd + 1), &m_rfds, &m_wfds, &m_efds, &timeout);

	if (numEvents > 0)
	{
		LOG_TRACE << numEvents << " events happened";
		fillActiveChannels(numEvents, activeChannels);
	}
	else if (numEvents == 0)
	{
		LOG_TRACE << "nothing happened";
	}
	else
	{
		if (sz_getlasterr() != sz_err_eintr)
		{
			LOG_SYSERR << "SelectPoller::poll()";
		}
	}
	return;
}

void SelectPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
	for (auto it = m_pollfds.begin(); it != m_pollfds.end() && numEvents > 0; ++it)
	{
		unsigned int revents = 0;
		if (FD_ISSET(it->fd, &m_rfds))
		{
			revents |= Channel::kReadEvent;
		}
		if (FD_ISSET(it->fd, &m_wfds))
		{
			revents |= Channel::kWriteEvent;
		}
		if (FD_ISSET(it->fd, &m_efds))
		{
			revents |= Channel::kErrorEvent;
		}
		if (revents > 0)
		{
			--numEvents;
			ChannelMap::const_iterator ch = m_channels.find(it->fd);
			assert(ch != m_channels.end());
			Channel* channel = ch->second;
			assert(channel->fd() == it->fd);
			channel->set_revents(revents);
			activeChannels->push_back(channel);
		}
	}
}

void SelectPoller::updateChannel(Channel* channel)
{
	Poller::assertInLoopThread();
	LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
	// add
	if (channel->index() < 0)
	{
		assert(m_channels.find(channel->fd()) == m_channels.end());
		Channel::Event_t event;
		event.fd = channel->fd();
		event.ev = channel->events();
		m_pollfds.push_back(event);
		int idx = static_cast<int>(m_pollfds.size()) - 1;
		channel->set_index(idx);
		m_channels[event.fd] = channel;
	}
	// mod
	else
	{
		// channel事件有能为kNoneEvent
		assert(m_channels.find(channel->fd()) != m_channels.end());
		assert(m_channels[channel->fd()] == channel);
		int idx = channel->index();
		assert(0 <= idx && idx < static_cast<int>(m_pollfds.size()));
		Channel::Event_t& event = m_pollfds[idx];
		assert(event.fd == channel->fd());
		event.ev = channel->events();
	}
}

void SelectPoller::removeChannel(Channel* channel)
{
	Poller::assertInLoopThread();
	LOG_TRACE << "fd = " << channel->fd();
	assert(m_channels.find(channel->fd()) != m_channels.end());
	assert(m_channels[channel->fd()] == channel);
	assert(channel->isNoneEvent());
	int idx = channel->index();
	assert(0 <= idx && idx < static_cast<int>(m_pollfds.size()));
	const Channel::Event_t& event = m_pollfds[idx];
	assert(event.ev == channel->events());
	// 移除
	size_t n = m_channels.erase(channel->fd());
	assert(n == 1); 
	if (implicit_cast<size_t>(idx) == m_pollfds.size() - 1)
	{
		m_pollfds.pop_back();
	}
	else
	{
		// 这个事件复杂度应该是O(1)
		auto channelAtEndFd = m_pollfds.back().fd;
		// 不要影响其他channel的下标
		std::iter_swap(m_pollfds.begin() + idx, m_pollfds.end() - 1);
		// 因为交换了位置，所以index发生了变化
		m_channels[channelAtEndFd]->set_index(idx);
		m_pollfds.pop_back();
	}
}

} // end namespace net

} // end namespace sznet