// Comment: 倒计时计数器实现

#ifndef _SZNET_THREAD_COUNTDOWNLATCH_H_
#define _SZNET_THREAD_COUNTDOWNLATCH_H_

#include "../base/NonCopyable.h"
#include "Condition.h"
#include "Mutex.h"

namespace sznet
{

// 倒计时计数器
class CountDownLatch : NonCopyable
{
public:
	explicit CountDownLatch(int count);

	// 等待计数器为0
	void wait();
	// 计数器减1
	void countDown();
	// 获取当前计数器
	int getCount() const;

private:
	// 互斥量
	mutable MutexLock m_mutex;
	// 条件变量
	Condition m_condition;
	// 计数器
	int m_count;
};

} // end namespace sznet

#endif // _SZNET_THREAD_COUNTDOWNLATCH_H_
