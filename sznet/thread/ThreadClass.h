// Comment: 线程实现

#ifndef _SZNET_THREAD_THREADCLASS_H_
#define _SZNET_THREAD_THREADCLASS_H_

#include "../base/NonCopyable.h"
#include "Thread.h"
#include "CountDownLatch.h"
#include <atomic>
#include <functional>

namespace sznet
{

class Thread : NonCopyable
{
public:
	typedef std::function<void()> ThreadFunc;

public:
	explicit Thread(ThreadFunc, const string& name = string());
	~Thread();

	// 开启线程
	void start();
	// 等待
	int join();
	// 线程是否已经开启
	bool started() const 
	{ 
		return m_started; 
	}
	// 线程tid
	sz_pid_t tid() const 
	{ 
		return m_tid; 
	}
	// 线程名称
	const string& name() const 
	{ 
		return m_name; 
	}
	// 获取m_numCreated，用记录构造的次数，辅助生成线程名
	static int numCreated() 
	{ 
		return m_numCreated.load();
	}

private:
	// 设置默认线程名称
	void setDefaultName();

	// 线程是否开启
	bool m_started;
	// 是否处于等待
	bool m_joined;
	// 线程ID
	sz_thread_t m_pthreadId;
	// 线程tid
	sz_pid_t m_tid;
	// 线程工作函数
	ThreadFunc m_func;
	// 线程名称
	string m_name;
	// 计数器，用来等待线程创建
	CountDownLatch m_latch;
	// 初始化32位原子整数，用记录构造的次数，辅助生成线程名
	static std::atomic<int> m_numCreated;
};

} // end namespace sznet

#endif // _SZNET_THREAD_THREADCLASS_H_
