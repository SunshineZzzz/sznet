#ifndef _SZNET_DS_BLOCKINGQUEUE_H_
#define _SZNET_DS_BLOCKINGQUEUE_H_

#include "../base/NonCopyable.h"
#include "../thread/Condition.h"
#include "../thread/Mutex.h"

#include <deque>
#include <assert.h>

namespace sznet
{

// 无边界阻塞队列，生产者消费者队列
template<typename T>
class BlockingQueue : NonCopyable
{
public:
	BlockingQueue(): 
		m_mutex(),
		m_notEmpty(m_mutex),
		m_queue()
	{
	}

	// 生产者生产商品，并且通知消费者
	void put(const T& x)
	{
		MutexLockGuard lock(m_mutex);
		m_queue.push_back(x);
		m_notEmpty.notify();
	}
	// 生产者生产商品，并且通知消费
	void put(T&& x)
	{
		MutexLockGuard lock(m_mutex);
		m_queue.push_back(std::move(x));
		m_notEmpty.notify();
	}
	// 消费者消费产品
	T take()
	{
		MutexLockGuard lock(m_mutex);
		while (m_queue.empty())
		{
			m_notEmpty.wait();
		}
		assert(!m_queue.empty());
		T front(std::move(m_queue.front()));
		m_queue.pop_front();
		return front;
	}
	// 当前队列中产品的数量
	size_t size() const
	{
		MutexLockGuard lock(m_mutex);
		return m_queue.size();
	}

private:
	// 配合条件变量，保护m_queue
	mutable MutexLock m_mutex;
	// 生产者生产商品了，就通知消费者
	Condition m_notEmpty;
	// 双端队列容器
	std::deque<T> m_queue;
};

} // end namespace sznet

#endif // _SZNET_DS_BLOCKINGQUEUE_H_
