#include "LogFile.h"
#include "../io/FileUtil.h"
#include "../time/Time.h"
#include "../process/Process.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>

namespace sznet
{

LogFile::LogFile(const string& basename, size_t rollSize, bool threadSafe, int flushInterval) :
	m_basename(basename),
	m_rollSize(rollSize),
	m_flushInterval(flushInterval),
	// m_checkEveryN(checkEveryN),
	m_count(0),
	m_mutex(threadSafe ? new MutexLock : nullptr),
	m_startOfPeriod(0),
	m_lastRoll(0),
	m_lastFlush(0)
{
	assert(basename.find('/') == string::npos);
	rollFile();
}

LogFile::~LogFile() = default;

void LogFile::append(const char* logline, int len)
{
	if (m_mutex)
	{
		MutexLockGuard lock(*m_mutex);
		append_unlocked(logline, len);
	}
	else
	{
		append_unlocked(logline, len);
	}
}

void LogFile::flush()
{
	if (m_mutex)
	{
		MutexLockGuard lock(*m_mutex);
		m_file->flush();
	}
	else
	{
		m_file->flush();
	}
}

void LogFile::append_unlocked(const char* logline, int len)
{
	time_t now = ::time(nullptr);
	// 注意，这里先除KRollPerSeconds然后乘KPollPerSeconds表示对齐值KRollPerSeconds的整数倍，
	// 也就是事件调整到当天零点
	time_t thisPeriod = now / m_kRollPerSeconds * m_kRollPerSeconds;
	// 第二天了换一个文件
	if (thisPeriod != m_startOfPeriod)
	{
		rollFile();
	}
	// 写的太多了，换一个一个文件
	else if (m_file->writtenBytes() > m_rollSize)
	{
		rollFile();
	}
	++m_count;
	// FileUtil.h中的AppendFile的不加锁apeend方法
	m_file->append(logline, len);
	// 是时候刷新了
	if (now - m_lastFlush > m_flushInterval)
	{
		m_lastFlush = now;
		m_file->flush();
	}
}

void LogFile::rollFile()
{
	time_t now = 0;
	string filename = getLogFileName(m_basename, &now);
	time_t start = now / m_kRollPerSeconds * m_kRollPerSeconds;
	m_lastRoll = now;
	m_lastFlush = now;
	m_startOfPeriod = start;
	m_count = 0;
	m_file.reset(new FileUtil::AppendFile(filename));
}

string LogFile::getLogFileName(const string& basename, time_t* now)
{
	string filename;
	// 预分配内存
	filename.reserve(basename.size() + 64);
	filename = basename;

	char timebuf[32];
	struct tm tm;
	*now = time(nullptr);
	// 获取UTC时间，_r是线程安全
	sz_localtime(tm, *now);
	// 格式化时间，strftime函数
	strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
	// 拼接上时间
	filename += timebuf;
	// 拼接上主机名
	filename += sz_gethostname();

	char pidbuf[32];
	snprintf(pidbuf, sizeof pidbuf, ".%d", sz_process_getpid());
	// 拼接上进程ID
	filename += pidbuf;
	// 拼接上日志标记.log
	filename += ".log";
	return filename;
}

} // end namespace sznet

