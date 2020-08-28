#ifndef _SZNET_NET_TIMERQUEUE_H_
#define _SZNET_NET_TIMERQUEUE_H_

#include <set>
#include <vector>

#include "../base/NonCopyable.h"
#include "../ds/MinHeap.h"
#include "../base/Callbacks.h"
#include "Timestamp.h"

namespace sznet
{

namespace net
{

// 事件循环
class EventLoop;

} // end namespace net

// 定时器
class Timer;
// 唯一标识一个Timer定时器
class TimerId;

// 定时器队列，管理所有定时器
class TimerQueue : NonCopyable
{
public:
	explicit TimerQueue(net::EventLoop* loop);
	~TimerQueue();

	// 
	TimerId addTimer(const net::TimerCallback cb, Timestamp when, double interval);
	// 
	void cancel(TimerId timerId);
	// 
	void expiredProcess(Timestamp now);
	// 
	int earliestExpiredTime(Timestamp now);

private:
	// 
	typedef std::set<Timer*> TimerSet;
	// 
	void addTimerInLoop(Timer* timer);
	// 
	void cancelInLoop(TimerId timerId);

	// TimerQueue所属的EventLoop
	net::EventLoop* m_loop;
	// 
	TimerSet m_timerSet;
	// 使用最小堆，保存所有的定时任务，到期时间最早的在前面
	MinHeap<Timer> m_timerMinHeap;
};

} // end namespace sznet

#endif // _SZNET_NET_TIMERQUEUE_H_
