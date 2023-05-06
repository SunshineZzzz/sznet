#include "LogStream.h"
#include <limits>
#include <algorithm>
#include <assert.h>

namespace sznet
{

namespace detail
{
	const char digits[] = "9876543210123456789";
	const char* zero = digits + 9;
	const char digitsHex[] = "0123456789ABCDEF";
	// 高效率数字转字符串，负数也没有问题
	template<typename T>
	size_t convert(char buf[], T value)
	{
		T i = value;
		char* p = buf;
		// 201 分别取数字 1 0 2
		do
		{
			int lsd = static_cast<int>(i % 10);
			i /= 10;
			*p++ = zero[lsd];
		} while (i != 0);
		// 判断 value 是否为负数
		if (value < 0)
		{
			*p++ = '-';
		}
		*p = '\0';
		// 反转
		std::reverse(buf, p);
		// 返回转换后的长度
		return p - buf;
	}
	// 高效率16进制非负数字转字符串
	size_t convertHex(char buf[], uintptr_t value)
	{
		uintptr_t i = value;
		char* p = buf;
		do
		{
			int lsd = static_cast<int>(i % 16);
			i /= 16;
			*p++ = digitsHex[lsd];
		} while (i != 0);
		*p = '\0';
		std::reverse(buf, p);
		return p - buf;
	}
	// 显示实例化：
	// template 函数返回类型 函数名 <实例化的类型> (函数形参表); 注意这是声明语句，要以分号结束。
	template class FixedBuffer<kSmallBuffer>;
	template class FixedBuffer<kLargeBuffer>;
} // end namespace detail

template<typename T>
void LogStream::_formatInteger(T v)
{
	if (m_buffer.avail() >= kMaxNumericSize)
	{
		size_t len = detail::convert(m_buffer.current(), v);
		m_buffer.add(len);
	}
}

LogStream& LogStream::operator<<(short v)
{
	*this << static_cast<int>(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned short v)
{
	*this << static_cast<unsigned int>(v);
	return *this;
}

LogStream& LogStream::operator<<(int v)
{
	_formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
	_formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(long v)
{
	_formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
	_formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(long long v)
{
	_formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
	_formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(const void* p)
{
	// convert to number
	uintptr_t i = reinterpret_cast<uintptr_t>(p);
	if (m_buffer.avail() > kMaxNumericSize)
	{
		char * buf = m_buffer.current();
		buf[0] = '0';
		buf[1] = 'x';
		size_t len = detail::convertHex(buf + 2, i);
		m_buffer.add(len);
	}
	return *this;
}

// FIXME: replace this with Grisu3 by Florian Loitsch.
LogStream& LogStream::operator<<(double v)
{
	if (m_buffer.avail() > kMaxNumericSize)
	{
		int len = snprintf(m_buffer.current(), kMaxNumericSize, "%.12g", v);
		m_buffer.add(len);
	}
	return *this;
}

template<typename T>
Fmt::Fmt(const char* fmt, T val)
{
	static_assert(std::is_arithmetic<T>::value == true, "Must be arithmetic type");
	m_length = snprintf(m_buf, sizeof(m_buf), fmt, val);
	assert(static_cast<size_t>(m_length) < sizeof(m_buf));
}

// Explicit instantiations
template Fmt::Fmt(const char* fmt, char);
template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int);
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);
template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);

} // end namespace sznet
