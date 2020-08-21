#ifndef _SZNET_LOG_LOGGING_H_
#define _SZNET_LOG_LOGGING_H_

#include "LogStream.h"
#include "../time/Timestamp.h"

namespace sznet
{

// 日志
// Logger => Impl => LogStream => operator<< FixedBuffer => g_output => g_flush
// Logger类主要负责日志的级别等，它的内部嵌套类Impl则负责实际的实现。
// 使用时，首先构造一个匿名的Logger对象，然后调用stream()函数返回一个LogStream对象，
// LogStream对象再调用重载的<<运算符来输出日志。事实上，日志先输出到缓冲区，然后才输出到标准输出或文件或其他。
// 匿名的Logger对象在销毁时调用析构函数，析构函数调用g_output和g_flush输出到日志对应的设备。
class Logger
{
public:
	// 日志输出函数类型
	typedef void(*OutputFunc)(const char* msg, int len);
	// 日志刷新函数类型
	typedef void(*FlushFunc)();
	// 日志级别
	enum LogLevel
	{
		// 比DEBUG粒度更细的一些信息事件（开发过程中使用）
		TRACE,
		// 细粒度信息事件对调试应用程序是非常有帮助的（开发过程中使用）
		DEBUG,
		// 消息在粗粒度级别上突出强调应用程序的运行过程
		INFO,
		// 系统能正常运行，但可能会出现潜在错误的情形
		WARN,
		// 指出虽然发生错误事件，但仍然不影响系统的继续运行
		ERROR,
		// 指出每个严重的错误事件将会导致应用程序的退出
		FATAL,
		// 日志级别个数
		NUM_LOG_LEVELS,
	};
	// basename实现
	class SourceFile
	{
	public:
		template<int N>
		SourceFile(const char(&arr)[N]): 
			m_data(arr),
			m_size(N - 1)
		{
			const char* slash = strrchr(m_data, '/');
			if (slash)
			{
				m_data = slash + 1;
				m_size -= static_cast<int>(m_data - arr);
			}
		}
		explicit SourceFile(const char* filename): 
			m_data(filename)
		{
			const char* slash = strrchr(filename, '/');
			if (slash)
			{
				m_data = slash + 1;
			}
			m_size = static_cast<int>(strlen(m_data));
		}

		// 文件名不包括路径
		const char* m_data;
		// 文件名不包括路径的长度
		int m_size;
	};

public:
	Logger(SourceFile file, int line);
	Logger(SourceFile file, int line, LogLevel level);
	Logger(SourceFile file, int line, LogLevel level, const char* func);
	Logger(SourceFile file, int line, bool toAbort);
	~Logger();

	// 获取当前日志的缓冲区
	LogStream& stream() 
	{ 
		return m_impl.m_stream; 
	}
	// 获取全局日志等级
	static LogLevel logLevel();
	// 设置全局日志等级
	static void setLogLevel(LogLevel level);
	// 设置全局日志输出函数
	static void setOutput(OutputFunc);
	// 设置全局日志缓冲区刷新函数
	static void setFlush(FlushFunc);

private:
	// 内部嵌套类，真正的实现
	class Impl
	{
	public:
		typedef Logger::LogLevel LogLevel;
		Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
		// 格式化时间(20180426 14:50:34.345346)放入缓冲(m_stream)
		void formatTime();
		// 结束字符串放入缓冲(-文件名:行号)
		void finish();

		// 当前时间戳
		Timestamp m_time;
		// 缓冲区
		LogStream m_stream;
		// 日志等级
		LogLevel m_level;
		// 行号
		int m_line;
		// 文件名
		SourceFile m_basename;
	};
	Impl m_impl;
};

// 全局日志等级
extern Logger::LogLevel g_logLevel;
// 获取当前的日志等级
inline Logger::LogLevel Logger::logLevel()
{
	return g_logLevel;
}

// 各种日志宏
#define LOG_TRACE if (sznet::Logger::logLevel() <= sznet::Logger::TRACE) sznet::Logger(__FILE__, __LINE__, sznet::Logger::TRACE, __func__).stream()
#define LOG_DEBUG if (sznet::Logger::logLevel() <= sznet::Logger::DEBUG) sznet::Logger(__FILE__, __LINE__, sznet::Logger::DEBUG, __func__).stream()
#define LOG_INFO if (sznet::Logger::logLevel() <= sznet::Logger::INFO) sznet::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN sznet::Logger(__FILE__, __LINE__, sznet::Logger::WARN).stream()
#define LOG_ERROR sznet::Logger(__FILE__, __LINE__, sznet::Logger::ERROR).stream()
#define LOG_FATAL sznet::Logger(__FILE__, __LINE__, sznet::Logger::FATAL).stream()
#define LOG_SYSERR sznet::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL sznet::Logger(__FILE__, __LINE__, true).stream()
// 线程安全根据errno获取错误字符串
const char* strerror_tl(int savedErrno);

// 用来检查某个对象是否为nullptr，为nullptr，打印日志，并且abort
#define CHECK_NOTNULL(val) ::sznet::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))
template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char* names, T* ptr)
{
	if (ptr == nullptr)
	{
		Logger(file, line, Logger::FATAL).stream() << names;
	}
	return ptr;
}

} // end namespace sznet

#endif // _SZNET_LOG_LOGGING_H_
