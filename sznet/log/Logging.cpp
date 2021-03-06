﻿#include "Logging.h"
#include "../thread/CurrentThread.h"
#include "../time/Timestamp.h"
#include "../time/Time.h"
#include "../process/Process.h"

#include <thread>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sstream>

namespace sznet
{

// 根据errno转换的错误字符串
thread_local char t_errnobuf[512];
// 日志时间格式：%4d%02d%02d %02d:%02d:%02d
// 20180426 14:50:34 
thread_local char t_time[64];
// 上一次记录日志的时间戳
// 同1s内打印的日志，只有微秒部分是需要被格式化的。避免反复格式化
thread_local time_t t_lastSecond;

// 线程安全根据errno获取错误字符串(C runtime library)
const char* strerror_tl(int savedErrno)
{
#if defined(SZ_OS_WINDOWS)
	// strerror_s(t_errnobuf, sizeof(t_errnobuf), savedErrno);
	auto chars = FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		savedErrno,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)t_errnobuf,
		sizeof(t_errnobuf) - 1,
		nullptr);
	if (chars != 0)
	{
		if (chars >= 2 && t_errnobuf[chars - 2] == '\r' && t_errnobuf[chars - 1] == '\n')
		{
			chars -= 2;
			t_errnobuf[chars] = 0;
		}
	}
#elif defined(SZ_OS_LINUX)
	strerror_r(savedErrno, t_errnobuf, sizeof(t_errnobuf));
#endif
	return t_errnobuf;
}

// 根据环境变量获取日志等级
Logger::LogLevel initLogLevel()
{
	if (::getenv("SZNET_LOG_TRACE"))
	{
		return Logger::TRACE;
	}
	else if (::getenv("SZNET_LOG_DEBUG"))
	{
		return Logger::DEBUG;
	}
	else
	{
		return Logger::INFO;
	}
}
// 全局日志等级
Logger::LogLevel g_logLevel = initLogLevel();

// 日志级别的string形式，方便输出到缓存
const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
{
	"TRACE ",
	"DEBUG ",
	"INFO  ",
	"WARN  ",
	"ERROR ",
	"FATAL ",
};

// helper class for known string length at compile time
// T类把char*和对应的len整合
class T
{
public:
	T(const char* str, unsigned len):
		m_str(str),
		m_len(len)
	{
		assert(strlen(str) == m_len);
	}

	const char* m_str;
	const unsigned m_len;
};

// T(已经格式化好的字符串)放入LogStream(缓冲区)
inline LogStream& operator<<(LogStream& s, T v)
{
	s.append(v.m_str, v.m_len);
	return s;
}

// Logger::SourceFile(文件名)放入LogStream(缓冲区)
inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v)
{
	s.append(v.m_data, v.m_size);
	return s;
}

// 默认日志输出函数
void defaultOutput(const char* msg, int len)
{
	size_t n = fwrite(msg, 1, len, stdout);
	(void)n;
}

// 默认缓冲区刷新函数
void defaultFlush()
{
	fflush(stdout);
}

// 全局日志输出函数
Logger::OutputFunc g_output = defaultOutput;
// 全局日志缓冲区刷新函数
Logger::FlushFunc g_flush = defaultFlush;

Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line): 
	m_time(Timestamp::now()),
	m_stream(),
	m_level(level),
	m_line(line),
	m_basename(file)
{
	// 写入时间字符串
	formatTime();
	// 尝试获取线程id
	CurrentThread::tid();
	// 写入线程id字符串
	m_stream << T(CurrentThread::tidString(), CurrentThread::tidStringLength());
	// 写入日志等级字符串
	m_stream << T(LogLevelName[level], 6);
	// 有errno，写入错误原因
	if (savedErrno != 0)
	{
		m_stream << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
	}
}

void Logger::Impl::formatTime()
{
	int64_t microSecondsSinceEpoch = m_time.microSecondsSinceEpoch();
	time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);
	int microSeconds = static_cast<int>(microSecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);
	if (seconds != t_lastSecond)
	{
		t_lastSecond = seconds;
		struct tm tm_time;
		sz_localtime(tm_time, seconds);
		int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
			tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
			tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
		assert(len == 17);
		(void)len;
	}
	Fmt us(".%06d ", microSeconds);
	assert(us.length() == 8);
	m_stream << T(t_time, 17) << T(us.data(), 8);
}

void Logger::Impl::finish()
{
	m_stream << " - " << m_basename << ':' << m_line << '\n';
}

Logger::Logger(SourceFile file, int line): 
	m_impl(INFO, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func): 
	m_impl(level, 0, file, line)
{
	m_impl.m_stream << func << ' ';
}

Logger::Logger(SourceFile file, int line, LogLevel level): 
	m_impl(level, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, bool toAbort): 
	m_impl(toAbort ? FATAL : ERROR, sz_getlasterr(), file, line)
{
}

Logger::~Logger()
{
	m_impl.finish();
	const LogStream::Buffer& buf(stream().buffer());
	g_output(buf.data(), buf.length());
	if (m_impl.m_level == FATAL)
	{
		g_flush();
		abort();
	}
}

void Logger::setLogLevel(Logger::LogLevel level)
{
	g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)
{
	g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
	g_flush = flush;
}

} // end namespace sznet
