#ifndef _SZNET_LOG_ASYNCLOGGING_H_
#define _SZNET_LOG_ASYNCLOGGING_H_

#include "../base/NonCopyable.h"
#include "../ds/BlockingQueue.h"
#include "../thread/Mutex.h"
#include "../thread/ThreadClass.h"
#include "../log/LogStream.h"

#include <atomic>
#include <vector>

namespace sznet
{

class AsyncLogging : NonCopyable
{
public:
	AsyncLogging(const string& basename, size_t rollSize, int flushInterval = 3);
	~AsyncLogging()
	{
		if (m_running)
		{
			stop();
		}
	}
	// 前端写日志消息到m_currentBuffer缓冲并在写满时通知线程
	void append(const char* logline, int len);
	// 启动线程
	void start()
	{
		m_running = true;
		// 日志线程启动
		m_thread.start();
		// 等待线程已经启动，才继续往下执行
		m_latch.wait();
	}
	// 线程停止
	void stop()
	{
		m_running = false;
		// 通知一下子线程
		m_cond.notify();
		// 等待子线程停止
		m_thread.join();
	}

private:
	// 线程处理函数(将数据写到日志文件)
	void threadFunc();

	// 固定大小为4M的Buffer
	typedef detail::FixedBuffer<detail::kLargeBuffer> Buffer;
	// buffer容器，自动管理动态内存的生命期
	typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
	// buffer容器元素类型，自动管理自身的生命期 std::unique_ptr<Buffer>
	typedef BufferVector::value_type BufferPtr;

	// 超时时间，在超时时间内前端没有写满，
	// 也要将这块缓冲区的数据添加到文件当中
	const int m_flushInterval;
	// 线程是否处于工作状态
	std::atomic<bool> m_running;
	// 日志文件名称前缀
	const string m_basename;
	// 日志文件写入大小达到rollSize换一个文件
	const size_t m_rollSize;
	// 使用了一个单独的线程来记录日志
	Thread m_thread;
	// 用于等待线程启动
	CountDownLatch m_latch;
	// 配合条件变量，保护 m_currentBuffer，m_nextBuffer，m_buffers
	MutexLock m_mutex;
	// 条件变量
	Condition m_cond;
	// 前端当前缓冲
	BufferPtr m_currentBuffer;
	// 前端预备缓冲
	BufferPtr m_nextBuffer;
	// 前端已经填满缓冲区的容器，供后端写入文件
	BufferVector m_buffers;
};

} // end namespace sznet

#endif // _SZNET_LOG_ASYNCLOGGING_H_
