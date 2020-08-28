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

// �¼�ѭ��
class EventLoop;

} // end namespace net

// ��ʱ��
class Timer;
// Ψһ��ʶһ��Timer��ʱ��
class TimerId;

// ��ʱ�����У��������ж�ʱ��
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

	// TimerQueue������EventLoop
	net::EventLoop* m_loop;
	// 
	TimerSet m_timerSet;
	// ʹ����С�ѣ��������еĶ�ʱ���񣬵���ʱ���������ǰ��
	MinHeap<Timer> m_timerMinHeap;
};

} // end namespace sznet

#endif // _SZNET_NET_TIMERQUEUE_H_
