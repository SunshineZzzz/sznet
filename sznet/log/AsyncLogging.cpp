#include "AsyncLogging.h"
#include "LogFile.h"
#include "../time/Timestamp.h"

#include <stdio.h>

namespace sznet
{

AsyncLogging::AsyncLogging(const string& basename, size_t rollSize, int flushInterval) :
	m_flushInterval(flushInterval),
	m_running(false),
	m_basename(basename),
	m_rollSize(rollSize),
	m_thread(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
	m_latch(1),
	m_mutex(),
	m_cond(m_mutex),
	m_currentBuffer(new Buffer),
	m_nextBuffer(new Buffer),
	m_buffers()
{
	m_currentBuffer->bzero();
	m_nextBuffer->bzero();
	m_buffers.reserve(16);
}

void AsyncLogging::append(const char* logline, int len)
{
	if (len > detail::kLargeBuffer)
	{
		assert(0);
	}

	MutexLockGuard lock(m_mutex);
	if (m_currentBuffer->avail() > len)
	{
		m_currentBuffer->append(logline, len);
	}
	else
	{
		// 当前m_currentBuffer不足够写入，
		// 就先把m_currentBuffer记录的日志消息移入待写入文件的容器
		m_buffers.push_back(std::move(m_currentBuffer));

		if (m_nextBuffer)
		{
			// 把预备好的另一块缓冲m_nextBuffer移用为当前缓冲
			m_currentBuffer = std::move(m_nextBuffer);
		}
		else
		{
			// Rarely happens
			// 如果前端写入太快，导致m_currentBuffer和m_nextBuffer缓冲都用完了
			// 那就重现分配一块
			m_currentBuffer.reset(new Buffer);
		}
		m_currentBuffer->append(logline, len);
		// 通知线程
		m_cond.notify();
	}
}

void AsyncLogging::threadFunc()
{
	assert(m_running == true);
	// 告诉主线程，子线程建立好了
	m_latch.countDown();
	// 创建一个把日志记录到文件的对象
	LogFile output(m_basename, m_rollSize, false);
	// 用来交换前端当前正在写的缓冲区
	BufferPtr newBuffer1(new Buffer);
	// 用来交换前端预备缓冲区(前提是预备缓冲区已经被前端征用)
	BufferPtr newBuffer2(new Buffer);
	newBuffer1->bzero();
	newBuffer2->bzero();
	// 用来交换前端容器
	BufferVector buffersToWrite;
	buffersToWrite.reserve(16);

	while (m_running)
	{
		assert(newBuffer1 && newBuffer1->length() == 0);
		assert(newBuffer2 && newBuffer2->length() == 0);
		assert(buffersToWrite.empty());

		{
			sznet::MutexLockGuard lock(m_mutex);
			if (m_buffers.empty())
			{
				m_cond.waitForSeconds(m_flushInterval);
			}
			m_buffers.push_back(std::move(m_currentBuffer));
			m_currentBuffer = std::move(newBuffer1);
			buffersToWrite.swap(m_buffers);
			if (!m_nextBuffer)
			{
				m_nextBuffer = std::move(newBuffer2);
			}
		}

		assert(!buffersToWrite.empty());
		// 消息堆积
		// 前端陷入死循环，拼命发送日志消息，超过后端的处理能力，这就是典型的生产速度
		// 超过消费速度的问题，会造成数据在内存中堆积，严重时引发性能问题（可用内存不足）
		// 或程序崩溃（分配内存失败）
		if (buffersToWrite.size() > 25)
		{
			char buf[256];
			snprintf(buf, sizeof(buf), "Dropped log messages at %s, %zd larger buffers\n",
				Timestamp::now().toFormattedString().c_str(),
				buffersToWrite.size() - 2);
			fputs(buf, stderr);
			output.append(buf, static_cast<int>(strlen(buf)));
			// 仅保留前两块缓冲区的日志消息，丢掉多余日志，以腾出内存
			buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
		}
		// 写入文件
		for (const auto& buffer : buffersToWrite)
		{
			// FIXME: use unbuffered stdio FILE ? or use ::writev ?
			output.append(buffer->data(), buffer->length());
		}
		// 仅保留两个buffer，用于newBuffer1 newBuffer2
		if (buffersToWrite.size() > 2)
		{
			// drop non-bzero-ed buffers, avoid trashing
			buffersToWrite.resize(2);
		}
		// 重置newBuffer1
		if (!newBuffer1)
		{
			assert(!buffersToWrite.empty());
			newBuffer1 = std::move(buffersToWrite.back());
			buffersToWrite.pop_back();
			newBuffer1->reset();
		}
		// 重置newBuffer2
		if (!newBuffer2)
		{
			assert(!buffersToWrite.empty());
			newBuffer2 = std::move(buffersToWrite.back());
			buffersToWrite.pop_back();
			newBuffer2->reset();
		}
		// 清空所有
		buffersToWrite.clear();
		// 刷新流
		output.flush();
	}
	// 线程结束前，刷新流
	output.flush();
}

} // end namespace sznet

