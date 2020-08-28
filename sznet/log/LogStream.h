#ifndef _SZNET_LOG_LOGSTREAM_H_
#define _SZNET_LOG_LOGSTREAM_H_

#include "../string/StringPiece.h"
#include "../base/NonCopyable.h"

#include <string.h> 
#include <assert.h>

namespace sznet
{

namespace detail
{

// 4k - 缓存最小size
const int kSmallBuffer = 4000;
// 4M - 缓存最大size
const int kLargeBuffer = 4000 * 1000;
// template non-type parameter
// FixedBuffer的实现为一个非类型参数的模板类，可以传入一个非类型参数SIZE表示缓冲区的大小。
// 一个固定大小SIZE的Buffer类
template<int SIZE>
class FixedBuffer : sznet::NonCopyable
{
public:
	FixedBuffer():
		m_cur(m_data)
	{
	}
	~FixedBuffer()
	{
	}

	// 缓冲区追加字符串
	// 如果可用数据足够，就拷贝buf过去，同时移动当前指针
	void append(const char* buf, size_t len)
	{
		if ((implicit_cast<size_t>(avail()) > len))
		{
			memcpy(m_cur, buf, len);
			m_cur += len;
		}
	}
	// 返回缓冲区首地址
	const char* data() const 
	{ 
		return m_data;
	}
	// 返回缓冲区已有数据长度
	int length() const 
	{ 
		return static_cast<int>(m_cur - m_data);
	}
	// 返回当前可写位置
	char* current() const 
	{ 
		return m_cur;
	}
	// 返回剩余可用BUFF长度
	int avail() const 
	{ 
		return static_cast<int>(_end() - m_cur);
	}
	// cur前移，写了多少增加多少
	void add(size_t len) 
	{ 
		m_cur += len;
	}
	// 重置，不清数据，只需要让cur指回首地址即可
	void reset() 
	{ 
		m_cur = m_data;
	}
	// 清零
	void bzero()
	{
		memset(m_data, 0, sizeof(m_data));
	}
	// 返回string
	string toString() const
	{
		return string(m_data, length());
	}
	// 返回StringPiece
	StringPiece toStringPiece() const
	{
		return StringPiece(m_data, length());
	}

private:
	// 返回结尾地址
	const char* _end() const
	{
		return m_data + sizeof(m_data);
	}

	// 缓冲区数据，大小为size
	char m_data[SIZE];
	// 当前可写位置的指针，永远指向已有数据的最右端
	char* m_cur;
}; // end class FixedBuffer

} // end namespace detail

// 重载了各种<<，负责把各个类型的数据转换成字符串，
// 再添加到FixedBuffer中
// 该类主要负责将要记录的日志内容放到这个Buffer里面
class LogStream : sznet::NonCopyable
{
private:
	typedef LogStream self;

public:
	typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;
	// 把下面各个类型转换为字符串后存储到buffer中
	self& operator<<(bool v)
	{
		m_buffer.append(v ? "1" : "0", 1);
		return *this;
	}
	self& operator<<(short);
	self& operator<<(unsigned short);
	self& operator<<(int);
	self& operator<<(unsigned int);
	self& operator<<(long);
	self& operator<<(unsigned long);
	self& operator<<(long long);
	self& operator<<(unsigned long long);
	self& operator<<(const void*);
	self& operator<<(double);
	self& operator<<(float v)
	{
		*this << static_cast<double>(v);
		return *this;
	}
	self& operator<<(char v)
	{
		m_buffer.append(&v, 1);
		return *this;
	}
	self& operator<<(const char* str)
	{
		if (str)
		{
			m_buffer.append(str, strlen(str));
		}
		else
		{
			m_buffer.append("(null)", 6);
		}
		return *this;
	}
	self& operator<<(const unsigned char* str)
	{
		return operator<<(reinterpret_cast<const char*>(str));
	}
	self& operator<<(const string& v)
	{
		m_buffer.append(v.c_str(), v.length());
		return *this;
	}
	self& operator<<(const StringPiece& v)
	{
		m_buffer.append(v.data(), v.size());
		return *this;
	}
	self& operator<<(const Buffer& v)
	{
		*this << v.toString();
		return *this;
	}
	// 追加字符串
	void append(const char* data, int len)
	{
		m_buffer.append(data, len);
	}
	// 返回buffer_对象
	const Buffer& buffer() const 
	{ 
		return m_buffer; 
	}
	// 重置buffer_ 
	void resetBuffer() 
	{
		m_buffer.reset(); 
	}

private:
	// 将整数转化为字符串放入缓冲区
	template<typename T>
	void _formatInteger(T);
	// 缓冲区
	Buffer m_buffer;
	// numeric需要的最大字节数
	static const int kMaxNumericSize = 32;
}; // end class LogStream

// 使用snprintf来格式化（format）成字符串
// 格式化字符串，从而方便缓存LogStream
class Fmt
{
public:
	template<typename T>
	Fmt(const char* fmt, T val);
	const char* data()const 
	{ 
		return m_buf;
	}
	int length() const 
	{ 
		return m_length;
	}

private:
	char m_buf[32];
	int m_length;
}; // end class Fmt

inline LogStream& operator<<(LogStream& s, const Fmt& fmt)
{
	s.append(fmt.data(), fmt.length());
	return s;
}

} // end namespace sznet

#endif // _SZNET_LOG_LOGSTREAM_H_
