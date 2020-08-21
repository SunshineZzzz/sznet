#ifndef _SZNET_TIME_TIMERID_H_
#define _SZNET_TIME_TIMERID_H_

#include <inttypes.h>
#include "../base/Copyable.h"

namespace sznet
{

// timer��ʱ��
class Timer;

// ����Ψһ��ʶ��Timer����Ҫ����ȡ��Timer 
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

	// 
	friend class TimerQueue;

private:
	// timer��ʱ��
	Timer* m_timer;
	// timer��ʱ�������к�
	int64_t m_sequence;
};

} // end namespace sznet

#endif // _SZNET_TIME_TIMERID_H_
