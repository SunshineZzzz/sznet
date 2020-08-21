#ifndef _SZNET_TIME_TIMER_H_
#define _SZNET_TIME_TIMER_H_

#include <atomic>

#include "../ds/MinHeap.h"
#include "Timestamp.h"
#include "../base/NonCopyable.h"
#include "../base/Callbacks.h"

namespace sznet
{

// 定时器
class Timer : public MinHeapbaseElem, NonCopyable
{
public:
	Timer(net::TimerCallback cb, Timestamp when, double interval): 
		m_callback(std::move(cb)),
		m_expiration(when),
		m_interval(interval),
		m_repeat(interval > 0.0),
		m_sequence(m_s_numCreated++)
	{ 
	}

	// 到期后执行回调函数
	void run() const
	{
		m_callback();
	}
	// 获取到期时间戳
	Timestamp expiration() const 
	{ 
		return m_expiration;
	}
	// 是否周期性定时
	bool repeat() const 
	{ 
		return m_repeat;
	}
	// 返回定时器的序列号
	int64_t sequence() const 
	{ 
		return m_sequence;
	}
	// 重启定时器 
	void restart(Timestamp now);
	// 返回定时器序列号生成器
	static int64_t numCreated() 
	{ 
		return m_s_numCreated;
	}

private:
	// 到期回调函数
	const net::TimerCallback m_callback;
	// 到期时间戳 
	Timestamp m_expiration;
	// 时间间隔，如果是一次性定时器，该值为0
	const double m_interval;
	// 是否周期性定时
	const bool m_repeat;
	// 本定时任务的序号
	const int64_t m_sequence;
	// 定时器序列号生成器，同样用来记录已经创建的定时器数量
	static std::atomic_int64_t m_s_numCreated;
};

} // end namespace sznet

#endif // _SZNET_TIME_TIMER_H_
