// Comment: 线程池事件IO循环实现

#ifndef _SZNET_NET_EVENTLOOPTHREADPOOL_H_
#define _SZNET_NET_EVENTLOOPTHREADPOOL_H_

#include "../NetCmn.h"
#include "../base/NonCopyable.h"

#include <functional>
#include <memory>
#include <vector>

namespace sznet
{

namespace net
{

// 事件IO循环
class EventLoop;
// 线程事件IO循环
class EventLoopThread;

// 线程池事件IO循环
class EventLoopThreadPool : NonCopyable
{
public:
	// 线程IO事件循环前的回调函数类型
	typedef std::function<void(EventLoop*)> ThreadInitCallback;

	EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg);
	~EventLoopThreadPool();

	// 设置IO事件循环的线程数量
	void setThreadNum(int numThreads) 
	{ 
		m_numThreads = numThreads; 
	}
	// 启动线程池
	void start(const ThreadInitCallback& cb = ThreadInitCallback());

	// 轮询获取事件IO循环
	EventLoop* getNextLoop();
	// 和线城池容量进行hash获取事件IO循环
	EventLoop* getLoopForHash(size_t hashCode);
	// 获取所有事件IO循环对象
	std::vector<EventLoop*> getAllLoops();
	// 返回是否启动
	bool started() const
	{
		return m_started;
	}
	// 返回名称前缀
	const string& name() const
	{
		return m_name;
	}

private:
	// acceptor所属的事件IO循环
	EventLoop* m_baseLoop;
	// 线程名称前缀
	string m_name;
	// 是否启动
	bool m_started;
	// 线程数量
	int m_numThreads;
	// 新连接到来，所选择的EventLoop对象下标
	int m_next;
	// 线程事件IO循环指针数组
	std::vector<std::unique_ptr<EventLoopThread>> m_threads;
	// 事件IO循环指针数组
	std::vector<EventLoop*> m_loops;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_EVENTLOOPTHREADPOOL_H_
