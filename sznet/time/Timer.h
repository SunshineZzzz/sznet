#ifndef _SZNET_TIME_TIMER_H_
#define _SZNET_TIME_TIMER_H_

#include <atomic>

#include "../ds/MinHeap.h"
#include "Timestamp.h"
#include "../base/NonCopyable.h"
#include "../base/Callbacks.h"

namespace sznet
{

// ��ʱ��
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

	// ���ں�ִ�лص�����
	void run() const
	{
		m_callback();
	}
	// ��ȡ����ʱ���
	Timestamp expiration() const 
	{ 
		return m_expiration;
	}
	// �Ƿ������Զ�ʱ
	bool repeat() const 
	{ 
		return m_repeat;
	}
	// ���ض�ʱ�������к�
	int64_t sequence() const 
	{ 
		return m_sequence;
	}
	// ������ʱ�� 
	void restart(Timestamp now);
	// ���ض�ʱ�����к�������
	static int64_t numCreated() 
	{ 
		return m_s_numCreated;
	}

private:
	// ���ڻص�����
	const net::TimerCallback m_callback;
	// ����ʱ��� 
	Timestamp m_expiration;
	// ʱ�����������һ���Զ�ʱ������ֵΪ0
	const double m_interval;
	// �Ƿ������Զ�ʱ
	const bool m_repeat;
	// ����ʱ��������
	const int64_t m_sequence;
	// ��ʱ�����к���������ͬ��������¼�Ѿ������Ķ�ʱ������
	static std::atomic_int64_t m_s_numCreated;
};

} // end namespace sznet

#endif // _SZNET_TIME_TIMER_H_
