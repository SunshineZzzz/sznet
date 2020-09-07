#include "Poller.h"
#include "SelectPoller.h"

#include <stdlib.h>

namespace sznet
{

namespace net
{

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
	return new SelectPoller(loop);
}

} // end namespace net

} // end namespace sznet
