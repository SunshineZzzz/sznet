#include "Timer.h"

namespace sznet
{

std::atomic_int64_t Timer::m_s_numCreated = 0;

void Timer::restart(Timestamp now)
{
	if (m_repeat)
	{
		// �����ظ�����һ�ε���ʱ��
		m_expiration = addTime(now, m_interval);
	}
	else
	{
		// �������ظ�����Ч��ʱ���
		m_expiration = Timestamp::invalid();
	}
}

} // end namespace sznet