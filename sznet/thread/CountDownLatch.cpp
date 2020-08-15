#include "CountDownLatch.h"

namespace sznet
{

CountDownLatch::CountDownLatch(int count) :
	m_mutex(),
	m_condition(m_mutex),
	m_count(count)
{
}

void CountDownLatch::wait()
{
	MutexLockGuard lock(m_mutex);
	while (m_count > 0)
	{
		m_condition.wait();
	}
}

void CountDownLatch::countDown()
{
	MutexLockGuard lock(m_mutex);
	--m_count;
	if (m_count == 0)
	{
		m_condition.notifyAll();
	}
}

int CountDownLatch::getCount() const
{
	MutexLockGuard lock(m_mutex);
	return m_count;
}

}