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

} // end namespace net

} // end namespace sznet 