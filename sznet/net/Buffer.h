// Comment: 网络消息缓冲区实现

#ifndef _SZNET_DS_BUFFER_H_
#define _SZNET_DS_BUFFER_H_

#include "../NetCmn.h"
#include "../base/Copyable.h"
#include "../string/StringPiece.h"
#include "SocketsOps.h"
#include "Endian.h"

#include <algorithm>
#include <vector>
#include <assert.h>
#include <string.h>

namespace sznet
{

namespace net
{

// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
//
// @code
// +-------------------+------------------+------------------+
// | prependable bytes |  readable bytes  |  writable bytes  |
// |                   |     (CONTENT)    |                  |
// +-------------------+------------------+------------------+
// |                   |                  |                  |
// 0      <=      readerIndex   <=   writerIndex    <=     size
// @endcode
// 网络消息缓冲区
class Buffer : public Copyable
{
public:
	// 预留位置，比如放置消息长度
	static const size_t kCheapPrepend = 4;
	// buffer缓冲区长度
	static const size_t kInitialSize = 1024;

	explicit Buffer(size_t nCheapPrepend = kCheapPrepend, size_t initialSize = kInitialSize):
		m_nCheapPrepend(nCheapPrepend),
		m_buffer(nCheapPrepend + initialSize),
		m_readerIndex(nCheapPrepend),
		m_writerIndex(nCheapPrepend),
		m_wrapSize(m_buffer.capacity() * 3 / 4)
	{
		assert(initialSize > 0);
		assert(nCheapPrepend >= 0);
		assert(readableBytes() == 0);
		assert(writableBytes() == initialSize);
		assert(prependableBytes() == nCheapPrepend);
	}

	// 交换
	void swap(Buffer& rhs)
	{
		m_buffer.swap(rhs.m_buffer);
		std::swap(m_readerIndex, rhs.m_readerIndex);
		std::swap(m_writerIndex, rhs.m_writerIndex);
	}
	// 可读数据长度
	size_t readableBytes() const
	{
		return m_writerIndex - m_readerIndex;
	}
	// 可写入数据长度
	size_t writableBytes() const
	{
		return m_buffer.size() - m_writerIndex;
	}
	// 返回当前可用预留位置长度
	size_t prependableBytes() const
	{
		return m_readerIndex;
	}
	// 返回可读数据起始位置，并不增加可读位置起始下标
	const char* peek() const
	{
		return begin() + m_readerIndex;
	}
	// 在未读内容中查找kCRLF字符串，返回位置指针
	const char* findCRLF() const
	{
		// FIXME: replace with memmem()?
		const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
		return crlf == beginWrite() ? nullptr : crlf;
	}
	const char* findCRLF(const char* start) const
	{
		assert(peek() <= start);
		assert(start <= beginWrite());
		// FIXME: replace with memmem()?
		const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
		return crlf == beginWrite() ? nullptr : crlf;
	}
	// 在未读内容中查找\n字符，返回位置指针
	const char* findEOL() const
	{
		const void* eol = memchr(peek(), '\n', readableBytes());
		return static_cast<const char*>(eol);
	}
	const char* findEOL(const char* start) const
	{
		assert(peek() <= start);
		assert(start <= beginWrite());
		const void* eol = memchr(start, '\n', beginWrite() - start);
		return static_cast<const char*>(eol);
	}
	// 调整m_readerIndex，后移len
	void retrieve(size_t len)
	{
		assert(len <= readableBytes());
		if (len < readableBytes())
		{
			m_readerIndex += len;
		}
		else
		{
			retrieveAll();
		}
	}
	// 调整m_readerIndex，后移end-peek()
	void retrieveUntil(const char* end)
	{
		assert(peek() <= end);
		assert(end <= beginWrite());
		retrieve(end - peek());
	}
	// 调整m_readerIndex，后移sizeof(int64_t)
	void retrieveInt64()
	{
		retrieve(sizeof(int64_t));
	}
	// 调整m_readerIndex，后移sizeof(int32_t)
	void retrieveInt32()
	{
		retrieve(sizeof(int32_t));
	}
	// 调整m_readerIndex，后移sizeof(int16_t)
	void retrieveInt16()
	{
		retrieve(sizeof(int16_t));
	}
	// 调整m_readerIndex，后移sizeof(int8_t)
	void retrieveInt8()
	{
		retrieve(sizeof(int8_t));
	}
	// 因为已经从缓冲区中读取所有数据，调整可读和可写位置到初始值
	void retrieveAll()
	{
		m_readerIndex = m_nCheapPrepend;
		m_writerIndex = m_nCheapPrepend;
	}
	// 从缓冲区中读取所有数据作为字符串返回，并且重置可读可写位置到初始值
	string retrieveAllAsString()
	{
		return retrieveAsString(readableBytes());
	}
	// 从缓冲区中读取len个字节的数据作为字符串返回，并且调整m_readerIndex，后移len
	string retrieveAsString(size_t len)
	{
		assert(len <= readableBytes());
		string result(peek(), len);
		retrieve(len);
		return result;
	}
	// 从缓冲区中读取所有数据作为string_view返回
	StringPiece toStringPiece() const
	{
		return StringPiece(peek(), static_cast<int>(readableBytes()));
	}
	// 写入指定长度的数据到缓冲区
	void append(const char* data, size_t len)
	{
		ensureWritableBytes(len);
		char* pDst = beginWrite();
		memcpy(pDst, data, len);
		hasWritten(len);
	}
	void append(const StringPiece& str)
	{
		append(str.data(), str.size());
	}
	void append(const void* data, size_t len)
	{
		append(static_cast<const char*>(data), len);
	}
	// 确保可写足够len的空间
	void ensureWritableBytes(size_t len)
	{
		if (writableBytes() < len)
		{
			makeSpace(len);
		}
		assert(writableBytes() >= len);
	}
	// 返回可写起始位置
	char* beginWrite()
	{
		return begin() + m_writerIndex;
	}
	const char* beginWrite() const
	{
		return begin() + m_writerIndex;
	}
	// 调整m_writerIndex，后移len
	void hasWritten(size_t len)
	{
		assert(len <= writableBytes());
		m_writerIndex += len;
	}
	// 调整m_writerIndex，前移len
	void unwrite(size_t len)
	{
		assert(len <= readableBytes());
		m_writerIndex -= len;
	}
	// 写入网络字节序int64
	void appendInt64(int64_t x)
	{
		int64_t be64 = net::sockets::hostToNetwork64(x);
		append(&be64, sizeof(be64));
	}
	// 写入网络字节序int32
	void appendInt32(int32_t x)
	{
		int32_t be32 = net::sockets::hostToNetwork32(x);
		append(&be32, sizeof(be32));
	}
	// 写入网络字节序int16
	void appendInt16(int16_t x)
	{
		int16_t be16 = net::sockets::hostToNetwork16(x);
		append(&be16, sizeof(be16));
	}
	// 写入网络字节序int8
	void appendInt8(int8_t x)
	{
		append(&x, sizeof(x));
	}
	// 读取int64
	int64_t readInt64()
	{
		int64_t result = peekInt64();
		retrieveInt64();
		return result;
	}
	// 读取int32
	int32_t readInt32()
	{
		int32_t result = peekInt32();
		retrieveInt32();
		return result;
	}
	// 读取int16
	int16_t readInt16()
	{
		int16_t result = peekInt16();
		retrieveInt16();
		return result;
	}
	// 读取int8
	int8_t readInt8()
	{
		int8_t result = peekInt8();
		retrieveInt8();
		return result;
	}
	// 读取int64，不偏移
	int64_t peekInt64() const
	{
		assert(readableBytes() >= sizeof(int64_t));
		int64_t be64 = 0;
		::memcpy(&be64, peek(), sizeof(be64));
		return net::sockets::networkToHost64(be64);
	}
	// 读取int32，不偏移
	int32_t peekInt32() const
	{
		assert(readableBytes() >= sizeof(int32_t));
		int32_t be32 = 0;
		::memcpy(&be32, peek(), sizeof(be32));
		return net::sockets::networkToHost32(be32);
	}
	// 读取int16，不偏移
	int16_t peekInt16() const
	{
		assert(readableBytes() >= sizeof(int16_t));
		int16_t be16 = 0;
		::memcpy(&be16, peek(), sizeof(be16));
		return net::sockets::networkToHost16(be16);
	}
	// 读取int8，不偏移
	int8_t peekInt8() const
	{
		assert(readableBytes() >= sizeof(int8_t));
		int8_t x = *peek();
		return x;
	}
	// 写入网络字节序int64到当前预留位置
	void prependInt64(int64_t x)
	{
		int64_t be64 = net::sockets::hostToNetwork64(x);
		prepend(&be64, sizeof(be64));
	}
	// 写入网络字节序int32到当前预留位置
	void prependInt32(int32_t x)
	{
		int32_t be32 = net::sockets::hostToNetwork32(x);
		prepend(&be32, sizeof(be32));
	}
	// 写入网络字节序int16到当前预留位置
	void prependInt16(int16_t x)
	{
		int16_t be16 = net::sockets::hostToNetwork16(x);
		prepend(&be16, sizeof(be16));
	}
	// 写入网络字节序int8到当前预留位置
	void prependInt8(int8_t x)
	{
		prepend(&x, sizeof(x));
	}
	// 写入指定长度的数据到当前预留位置，从m_readerIndex后向写入
	void prepend(const void* data, size_t len)
	{
		assert(len <= prependableBytes());
		m_readerIndex -= len;
		const char* d = static_cast<const char*>(data);
		// std::copy(d, d + len, begin() + m_readerIndex);
		char* pDst = begin() + m_readerIndex;
		memcpy(pDst, d, len);
	}
	// shrink
	void shrink(size_t reserve)
	{
		// FIXME: use vector::shrink_to_fit() in C++ 11 if possible.
		Buffer other;
		other.ensureWritableBytes(readableBytes() + reserve);
		other.append(toStringPiece());
		swap(other);
		m_wrapSize = m_buffer.capacity() * 3 / 4;
	}
	// 内部buffer的capacity
	size_t internalCapacity() const
	{
		return m_buffer.capacity();
	}
	// 从socket中读取数据
	sz_ssize_t readvFd(sockets::sz_sock fd, int* savedErrno);
	sz_ssize_t readFd(sockets::sz_sock fd, int* savedErrno);
	sz_ssize_t recvFrom(sockets::sz_sock fd, struct sockaddr* addr, int* savedErrno);

private:
	// buffer缓冲区头
	char* begin()
	{
		return &*m_buffer.begin();
	}
	const char* begin() const
	{
		return &*m_buffer.begin();
	}
	// 回绕
	void wrapBuffer()
	{
		if (m_readerIndex == m_nCheapPrepend)
		{
			return;
		}

		// 可读数据长度
		size_t readable = readableBytes();
		// std::copy(begin() + m_readerIndex, begin() + m_writerIndex, begin() + m_nCheapPrepend);
		char* pDst = begin() + m_nCheapPrepend;
		char* pSrc = begin() + m_readerIndex;
		size_t writeSize = m_writerIndex - m_readerIndex;
		memmove(pDst, pSrc, writeSize);
		m_readerIndex = m_nCheapPrepend;
		m_writerIndex = m_readerIndex + readable;
		assert(readable == readableBytes());
	}
	// 回绕缓冲区或者增大
	void makeSpace(size_t len)
	{
		if (writableBytes() + prependableBytes() < len + m_nCheapPrepend)
		{
			// 缓冲区确实不够了
			// FIXME: move readable data
			m_buffer.resize(m_writerIndex + len);
			m_wrapSize = m_buffer.capacity() * 3 / 4;
		}
		else
		{
			// 回绕
			assert(m_nCheapPrepend < m_readerIndex);
			wrapBuffer();
		}
	}

private:
	// 预留长度
	const size_t m_nCheapPrepend;
	// 缓冲区
	std::vector<char> m_buffer;
	// 可读位置起始下标
	size_t m_readerIndex;
	// 可写位置起始下标
	size_t m_writerIndex;
	// 回绕大小
	size_t m_wrapSize;
	// 换行符
	static const char kCRLF[];
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_DS_BUFFER_H_
