#include "../log/Logging.h"
#include "Channel.h"
#include "EventLoop.h"
#include <sstream>

namespace sznet
{

namespace net
{

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = 0;
const int Channel::kWriteEvent = 0;

Channel::Channel(EventLoop* loop, int fd__): 
	m_loop(loop),
	m_fd(fd__),
	m_events(0),
	m_revents(0),
	m_index(-1),
	m_logHup(true),
	m_tied(false),
	m_eventHandling(false),
	m_addedToLoop(false)
{
}

Channel::~Channel()
{
	assert(!m_eventHandling);
	assert(!m_addedToLoop);
	if (m_loop->isInLoopThread())
	{
		// assert(!m_loop->hasChannel(this));
	}
}

void Channel::tie(const std::shared_ptr<void>& obj)
{
	m_tie = obj;
	m_tied = true;
}

void Channel::update()
{
	m_addedToLoop = true;
	// m_loop->updateChannel(this);
}

void Channel::remove()
{
	assert(isNoneEvent());
	m_addedToLoop = false;
	// m_loop->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
	std::shared_ptr<void> guard;
	if (m_tied)
	{
		guard = m_tie.lock();
		if (guard)
		{
			handleEventWithGuard(receiveTime);
		}
	}
	else
	{
		handleEventWithGuard(receiveTime);
	}
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
}

string Channel::reventsToString() const
{
	return eventsToString(m_fd, m_revents);
}

string Channel::eventsToString() const
{
	return eventsToString(m_fd, m_events);
}

string Channel::eventsToString(int fd, int ev)
{
	return "";
}

} // namespace net

} // namespace sznet