#include "TimerQueue.h"
#include "Timer.h"
#include "TimerId.h"
#include "../log/Logging.h"
#include "../net/EventLoop.h"

namespace sznet
{

// 用于最小堆的比较函数
// 1 - 左边大于右边， 0 - 左边等于右边， - 1 - 左边小于右边
int event_greater(Timer* lhs, Timer* rhs)
{
	auto key1 = lhs->expiration().microSecondsSinceEpoch();
	auto key2 = rhs->expiration().microSecondsSinceEpoch();
	if (key1 < key2)
	{
		return -1;
	}
	else if (key1 == key2)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

TimerQueue::TimerQueue(net::EventLoop* loop):
	m_loop(loop),
	m_timerSet(),
	m_timerMinHeap(event_greater)
{
}

TimerQueue::~TimerQueue()
{
	for (TimerSet::iterator it = m_timerSet.begin(); it != m_timerSet.end(); ++it)
	{
		delete *it;
	}
}

TimerId TimerQueue::addTimer(const net::TimerCallback cb, Timestamp when, double interval)
{
	Timer* timer = new Timer(cb, when, interval);
	// m_loop->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
	return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId)
{
	// m_loop->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
	// m_loop->assertInLoopThread();
	Timestamp when = timer->expiration();
	m_timerMinHeap.push(timer);
	m_timerSet.insert(timer);
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
	// m_loop->assertInLoopThread();
	m_timerMinHeap.erase(timerId.m_timer);
}

void TimerQueue::expiredProcess(Timestamp now)
{
	while (1)
	{
		Timer* timer = m_timerMinHeap.pop();
		if (!timer)
		{
			break;
		}
		if (timer->expiration().microSecondsSinceEpoch() > now.microSecondsSinceEpoch())
		{
			break;
		}
		// m_loop->runInLoop(std::bind(&Timer::run, timer));
		if (timer->repeat())
		{
			timer->restart(now);
			m_timerMinHeap.push(timer);
		}
	}
}

int TimerQueue::earliestExpiredTime(Timestamp now)
{
	return 0;
}

} // namespace sznet
