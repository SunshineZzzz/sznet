#ifndef _SZNET_BASE_EXCEPTION_H_
#define _SZNET_BASE_EXCEPTION_H_

#include "../NetCmn.h"
#include "../thread/CurrentThread.h"
#include <exception>


namespace sznet
{

// 异常基类
class Exception : public std::exception
{
public:
	Exception(string what) :
		m_message(std::move(what)),
		m_stack(CurrentThread::stackTrace(false))
	{
	}
	~Exception() noexcept override = default;

	// 返回错误原因
	const char* what() const noexcept override
	{
		return m_message.c_str();
	}
	// 返回调用堆栈
	const char* stackTrace() const noexcept
	{
		return m_stack.c_str();
	}

private:
	// 错误信息
	string m_message;
	// 调用栈
	string m_stack;
};

} // namespace sznet

#endif // _SZNET_BASE_EXCEPTION_H_
