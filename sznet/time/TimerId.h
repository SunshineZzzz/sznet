#ifndef _SZNET_TIME_TIMERID_H_
#define _SZNET_TIME_TIMERID_H_

#include <inttypes.h>
#include "../base/Copyable.h"

namespace sznet
{

// timer定时器
class Timer;

// 带有唯一标识的Timer
class TimerId : public Copyable
{
public:
	TimerId(): 
		m_timer(nullptr),
		m_sequence(0)
	{
	}
	TimerId(Timer* timer, int64_t seq): 
		m_timer(timer),
		m_sequence(seq)
	{
	}

	// 定时器队列
	friend class TimerQueue;

private:
	// timer定时器
	Timer* m_timer;
	// timer定时器的序列号
	int64_t m_sequence;
};

} // end namespace sznet

#endif // _SZNET_TIME_TIMERID_H_
