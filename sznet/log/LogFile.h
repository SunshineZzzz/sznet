#ifndef _SZNET_LOG_LOGFILE_H_
#define _SZNET_LOG_LOGFILE_H_

#include "../base/NonCopyable.h"
#include "../THREAD/Mutex.h"

#include <memory>

namespace sznet
{

namespace FileUtil
{
// 非线程安全，写文件封装
class AppendFile;
}

// 用于把日志记录到文件的类
class LogFile : NonCopyable
{
public:
	LogFile(
		const string& basename, // 日志文件名前缀
		size_t rollSize, // 日志文件写入大小达到rollSize换一个文件
		bool threadSafe = true, // 默认开启线程安全，即初始化mutex
		int flushInterval = 3); // flush间隔
		// int checkEveryN = 1024); // append文件上限，可以写逻辑
	~LogFile();

	// 追加内容到文件
	void append(const char* logline, int len);
	// 刷新到文件
	void flush();
	// 生成新文件
	void rollFile();

private:
	// 不加锁的append方式
	void append_unlocked(const char* logline, int len);
	// 生成日志文件的名称(运行程序.时间.主机名.进程ID.log)
	// logfile_test.20130411-115604.popo.7743.log
	static string getLogFileName(const string& basename, time_t* now);

	// 日志文件前缀名称
	const string m_basename;
	// 日志文件达到m_rollSize换一个新文件
	const size_t m_rollSize;
	// 日志写入文件超时间隔时间
	const int m_flushInterval;
	// 文件append次数上限，这个暂时就不用了
	// const int m_checkEveryN;
	// append次数
	int m_count;
	// 多线程写入，保护m_file
	std::unique_ptr<MutexLock> m_mutex;
	// 开始记录日志时间
	// (调整到这一天的零时时间, 时间戳/kRollPerSeconds_ * kRollPerSeconds_)
	time_t m_startOfPeriod;
	// 上一次更换日志文件时间，这个没有调整到这一天的零时
	time_t m_lastRoll;
	// 上一次日志写入(刷新)文件时间，更换文件也会修改
	time_t m_lastFlush;
	// 文件智能指针
	std::unique_ptr<FileUtil::AppendFile> m_file;
	// 一天的秒数
	const static int m_kRollPerSeconds = 60 * 60 * 24;
};

} // namespace sznet

#endif // _SZNET_LOG_LOGFILE_H_
