#include "Timer.h"

namespace sznet
{

std::atomic<std::uint64_t> Timer::m_s_numCreated(0);

void Timer::restart(Timestamp now)
{
	if (m_repeat)
	{
		// 可以重复，下一次到期时间
		m_expiration = addTime(now, m_interval);
	}
	else
	{
		// 不可以重复，无效的时间戳对象
		m_expiration = Timestamp::invalid();
	}
}

} // end namespace sznet