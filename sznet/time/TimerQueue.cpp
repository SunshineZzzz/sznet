#include "TimerQueue.h"
#include "Timer.h"
#include "TimerId.h"
#include "../log/Logging.h"
#include "../net/EventLoop.h"

#include <algorithm>

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
	m_timerMinHeap(event_greater)
{
}

TimerQueue::~TimerQueue()
{
	assert(m_timerMinHeap.size() == m_activeTimers.size());
	while (true)
	{
		Timer* timer = m_timerMinHeap.pop();
		if (!timer)
		{
			break;
		}
		delete timer;
		timer = nullptr;
	}
	m_activeTimers.clear();
	assert(m_timerMinHeap.size() == m_activeTimers.size());
}

TimerId TimerQueue::addTimer(const net::TimerCallback cb, Timestamp when, double interval)
{
	Timer* timer = new Timer(cb, when, interval);
	m_loop->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
	return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId)
{
	m_loop->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
	m_loop->assertInLoopThread();
	assert(m_timerMinHeap.size() == m_activeTimers.size());
	m_timerMinHeap.push(timer);
	std::pair<ActiveTimerSet::iterator, bool> result = m_activeTimers.insert(ActiveTimer(timer, timer->sequence()));
	assert(result.second); (void)result;
	assert(m_timerMinHeap.size() == m_activeTimers.size());
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
	m_loop->assertInLoopThread();
	assert(m_timerMinHeap.size() == m_activeTimers.size());
	ActiveTimer timer(timerId.m_timer, timerId.m_sequence);
	ActiveTimerSet::iterator it = m_activeTimers.find(timer);
	if (it != m_activeTimers.end())
	{
		assert(m_timerMinHeap.erase(timerId.m_timer) == 0);
		delete timerId.m_timer;
		timerId.m_timer = nullptr;
		m_activeTimers.erase(it);
	}
	assert(m_timerMinHeap.size() == m_activeTimers.size());
}

void TimerQueue::expiredProcess(Timestamp now)
{
	m_loop->assertInLoopThread();
	assert(m_timerMinHeap.size() == m_activeTimers.size());
	while (true)
	{
		Timer* timer = m_timerMinHeap.top();
		if (!timer)
		{
			break;
		}
		if (timer->expiration().microSecondsSinceEpoch() > now.microSecondsSinceEpoch())
		{
			break;
		}
		ActiveTimer activeTimer(timer, timer->sequence());
		m_timerMinHeap.pop();
		size_t n = m_activeTimers.erase(activeTimer);
		assert(n == 1); (void)n;
		// 执行超时函数
		timer->run();
		if (timer->repeat())
		{
			timer->restart(now);
			addTimerInLoop(timer);
		}
		else 
		{
			delete timer;
			timer = nullptr;
		}
	}
	assert(m_timerMinHeap.size() == m_activeTimers.size());
}

int TimerQueue::earliestExpiredTime(Timestamp now, int defaultTimeMs)
{
	Timer* timer = m_timerMinHeap.top();
	if (!timer)
	{
		return defaultTimeMs;
	}

	if (timer->expiration().microSecondsSinceEpoch() > now.microSecondsSinceEpoch())
	{
		return std::min(timeDifferenceMs(timer->expiration(), now), defaultTimeMs);
	}

	return defaultTimeMs;
}

} // namespace sznet
