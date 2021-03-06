﻿#ifndef _SZNET_NET_TIMERQUEUE_H_
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

	// 添加一个定时器
	// 一定是线程安全的，可以跨线程调用，通常情况下被其他线程调用
	TimerId addTimer(const net::TimerCallback cb, Timestamp when, double interval);
	// 取消timer
	// 一定是线程安全的，可以跨线程调用，通常情况下被其他线程调用
	void cancel(TimerId timerId);
	// 处理到时的timer，执行其回调函数
	void expiredProcess(Timestamp now);
	// 获取最早timer过期时间和now的差值，并且和defaultTimeMs比较，返回较小者
	int earliestExpiredTime(Timestamp now = Timestamp::now(), int defaultTimeMs = 0);

private:
	// IO循环中增加timer
	void addTimerInLoop(Timer* timer);
	// IO循环中取消timer
	void cancelInLoop(TimerId timerId);

	// 活跃timer类型
	typedef std::pair<Timer*, int64_t> ActiveTimer;
	typedef std::set<ActiveTimer> ActiveTimerSet;

	// TimerQueue所属的EventLoop
	net::EventLoop* m_loop;
	// 使用最小堆，保存所有的定时任务，到期时间最早的在前面
	MinHeap<Timer> m_timerMinHeap;
	// 当前活跃定时器
	// m_timerMinHeap 与 m_activeTimers 都保存了相同的Timer 地址
	// m_timerMinHeap 是按超时时间排序，m_activeTimers 是按定时器地址排序
	ActiveTimerSet m_activeTimers;
};

} // end namespace sznet

#endif // _SZNET_NET_TIMERQUEUE_H_
