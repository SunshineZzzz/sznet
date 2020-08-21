#ifndef _SZNET_DS_BOUNDEDBLOCKINGQUEUE_H_
#define _SZNET_DS_BOUNDEDBLOCKINGQUEUE_H_

#include "../base/NonCopyable.h"
#include "../thread/Condition.h"
#include "../thread/Mutex.h"

#include <deque>

namespace sznet
{

// 有界阻塞队列的操作，生产者消费者队列
template<typename T>
class BoundedBlockingQueue : NonCopyable
{
public:
	BoundedBlockingQueue(int maxQueueSize):
		m_mutex(),
		m_notFull(),
		m_notEmpty(),
		m_capacity(maxQueueSize)
	{
		assert(m_capacity > 0);
	}

	// 因为是有界限，所以如果满了，等待消费者消费后通知
	// 生产者生产商品，并且通知消费者
	void put(T& x)
	{
		MutexLockGuard lock(m_mutex);
		while (full())
		{
			m_notFull.wait();
		}
		m_queue.push_back(x);
		m_notEmpty.notify();
	}
	// 消费者消费产品
	// 因为是有界限，所以消费者消费商品后，通知生产者有空余
	T take()
	{
		MutexLockGuard lock(m_mutex);
		while (empty())
		{
			m_notEmpty.wait();
		}
		T front(m_queue.front());
		m_queue.pop_front();
		m_notFull.notify();
		return front;
	}
	// 缓冲区是否已满
	bool full() const
	{
		MutexLockGuard lock(m_mutex);
		return m_queue.size() == m_capacity;
	}
	// 缓冲区是否为空
	bool empty() const
	{
		MutexLockGuard lock(m_mutex);
		return m_queue.empty();
	}
	// 缓冲区数量
	size_t size() const
	{
		MutexLockGuard lock(m_mutex);
		return m_queue.size();
	}
	// 缓冲区容量
	size_t capacity() const 
	{ 
		MutexLockGuard lock(m_mutex);
		return m_capacity; 
	}

private:
	// 配合条件变量，保护m_queue
	mutable MutexLock m_mutex;
	// 消费者消费产品了，就通知生产者
	Condition m_notFull;
	// 生产者生产商品了，就通知消费者
	Condition m_notEmpty;
	// 缓冲区最大容量
	size_t m_capacity;
	// 双端队列容器
	std::deque<T> m_queue;
};

} // end namespace sznet

#endif // _SZNET_DS_BOUNDEDBLOCKINGQUEUE_H_