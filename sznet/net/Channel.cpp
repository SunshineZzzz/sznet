#include "../log/Logging.h"
#include "Channel.h"
#include "EventLoop.h"
#include <sstream>

namespace sznet
{

namespace net
{

Channel::Channel(EventLoop* loop, sockets::sz_fd fd):
	m_loop(loop),
	m_fd(fd),
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
		assert(!m_loop->hasChannel(this));
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
	m_loop->updateChannel(this);
}

void Channel::remove()
{
	assert(isNoneEvent());
	m_addedToLoop = false;
	m_loop->removeChannel(this);
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
	// 事件处理时，设置下此状态
	// Channel析构时，用到此状态 
	m_eventHandling = true;
	LOG_TRACE << reventsToString();
	// RCV_SHUTDOWN | SEND_SHUTDOWN
	if ((m_revents & kHupEvent) && !(m_revents & kReadEvent))
	{
		if (m_logHup)
		{
			LOG_WARN << "fd = " << m_fd << " Channel::handle_event() kHupEvent";
		}
		if (m_closeCallback)
		{
			m_closeCallback();
		}
	}

	// A POLLNVAL means the socket file descriptor is not open. 
	// It would be an error to close() it.
	if (m_revents & kNvalEvent)
	{
		LOG_WARN << "fd = " << m_fd << " Channel::handle_event() KNvalEvent";
	}

	// A POLLERR means error condition happened on the associated file descriptor.
	if (m_revents & (kErrorEvent | kNvalEvent))
	{
		if (m_errorCallback)
		{
			m_errorCallback();
		}
	}

	// 读事件
	if (m_revents & (kReadEvent | kPriEvent | kRdHupEvent))
	{
		if (m_readCallback)
		{
			m_readCallback(receiveTime);
		}
	}

	// 写事件
	if (m_revents & kWriteEvent)
	{
		if (m_writeCallback)
		{
			m_writeCallback();
		}
	}

	m_eventHandling = false;
}

string Channel::reventsToString() const
{
	return eventsToString(m_fd, m_revents);
}

string Channel::eventsToString() const
{
	return eventsToString(m_fd, m_events);
}

string Channel::eventsToString(sockets::sz_sock fd, int ev)
{
	std::ostringstream oss;
	oss << fd << ": ";
	if (ev & kReadEvent)
	{
		oss << "IN ";
	}
	if (ev & kPriEvent)
	{
		oss << "PRI ";
	}
	if (ev & kRdHupEvent)
	{
		oss << "RDHUP ";
	}
	if (ev & kWriteEvent)
	{
		oss << "OUT ";
	}
	if (ev & kErrorEvent)
	{
		oss << "ERROR ";
	}
	if (ev & kHupEvent)
	{
		oss << "HUP ";
	}
	if (ev & kNvalEvent)
	{
		oss << "NVAL ";
	}
	return oss.str();
}

} // namespace net

} // namespace sznet