// Comment: ������Ϣ������ʵ��

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
// ������Ϣ������
class Buffer : public Copyable
{
public:
	// Ԥ��λ�ã����������Ϣ����
	static const size_t kCheapPrepend = 4;
	// buffer����������
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

	// ����
	void swap(Buffer& rhs)
	{
		m_buffer.swap(rhs.m_buffer);
		std::swap(m_readerIndex, rhs.m_readerIndex);
		std::swap(m_writerIndex, rhs.m_writerIndex);
	}
	// �ɶ����ݳ���
	size_t readableBytes() const
	{
		return m_writerIndex - m_readerIndex;
	}
	// ��д�����ݳ���
	size_t writableBytes() const
	{
		return m_buffer.size() - m_writerIndex;
	}
	// ���ص�ǰ����Ԥ��λ�ó���
	size_t prependableBytes() const
	{
		return m_readerIndex;
	}
	// ���ؿɶ�������ʼλ�ã��������ӿɶ�λ����ʼ�±�
	const char* peek() const
	{
		return begin() + m_readerIndex;
	}
	// ��δ�������в���kCRLF�ַ���������λ��ָ��
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
	// ��δ�������в���\n�ַ�������λ��ָ��
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
	// ����m_readerIndex������len
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
	// ����m_readerIndex������end-peek()
	void retrieveUntil(const char* end)
	{
		assert(peek() <= end);
		assert(end <= beginWrite());
		retrieve(end - peek());
	}
	// ����m_readerIndex������sizeof(int64_t)
	void retrieveInt64()
	{
		retrieve(sizeof(int64_t));
	}
	// ����m_readerIndex������sizeof(int32_t)
	void retrieveInt32()
	{
		retrieve(sizeof(int32_t));
	}
	// ����m_readerIndex������sizeof(int16_t)
	void retrieveInt16()
	{
		retrieve(sizeof(int16_t));
	}
	// ����m_readerIndex������sizeof(int8_t)
	void retrieveInt8()
	{
		retrieve(sizeof(int8_t));
	}
	// ��Ϊ�Ѿ��ӻ������ж�ȡ�������ݣ������ɶ��Ϳ�дλ�õ���ʼֵ
	void retrieveAll()
	{
		m_readerIndex = m_nCheapPrepend;
		m_writerIndex = m_nCheapPrepend;
	}
	// �ӻ������ж�ȡ����������Ϊ�ַ������أ��������ÿɶ���дλ�õ���ʼֵ
	string retrieveAllAsString()
	{
		return retrieveAsString(readableBytes());
	}
	// �ӻ������ж�ȡlen���ֽڵ�������Ϊ�ַ������أ����ҵ���m_readerIndex������len
	string retrieveAsString(size_t len)
	{
		assert(len <= readableBytes());
		string result(peek(), len);
		retrieve(len);
		return result;
	}
	// �ӻ������ж�ȡ����������Ϊstring_view����
	StringPiece toStringPiece() const
	{
		return StringPiece(peek(), static_cast<int>(readableBytes()));
	}
	// д��ָ�����ȵ����ݵ�������
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
	// ȷ����д�㹻len�Ŀռ�
	void ensureWritableBytes(size_t len)
	{
		if (writableBytes() < len)
		{
			makeSpace(len);
		}
		assert(writableBytes() >= len);
	}
	// ���ؿ�д��ʼλ��
	char* beginWrite()
	{
		return begin() + m_writerIndex;
	}
	const char* beginWrite() const
	{
		return begin() + m_writerIndex;
	}
	// ����m_writerIndex������len
	void hasWritten(size_t len)
	{
		assert(len <= writableBytes());
		m_writerIndex += len;
	}
	// ����m_writerIndex��ǰ��len
	void unwrite(size_t len)
	{
		assert(len <= readableBytes());
		m_writerIndex -= len;
	}
	// д�������ֽ���int64
	void appendInt64(int64_t x)
	{
		int64_t be64 = net::sockets::hostToNetwork64(x);
		append(&be64, sizeof(be64));
	}
	// д�������ֽ���int32
	void appendInt32(int32_t x)
	{
		int32_t be32 = net::sockets::hostToNetwork32(x);
		append(&be32, sizeof(be32));
	}
	// д�������ֽ���int16
	void appendInt16(int16_t x)
	{
		int16_t be16 = net::sockets::hostToNetwork16(x);
		append(&be16, sizeof(be16));
	}
	// д�������ֽ���int8
	void appendInt8(int8_t x)
	{
		append(&x, sizeof(x));
	}
	// ��ȡint64
	int64_t readInt64()
	{
		int64_t result = peekInt64();
		retrieveInt64();
		return result;
	}
	// ��ȡint32
	int32_t readInt32()
	{
		int32_t result = peekInt32();
		retrieveInt32();
		return result;
	}
	// ��ȡint16
	int16_t readInt16()
	{
		int16_t result = peekInt16();
		retrieveInt16();
		return result;
	}
	// ��ȡint8
	int8_t readInt8()
	{
		int8_t result = peekInt8();
		retrieveInt8();
		return result;
	}
	// ��ȡint64����ƫ��
	int64_t peekInt64() const
	{
		assert(readableBytes() >= sizeof(int64_t));
		int64_t be64 = 0;
		::memcpy(&be64, peek(), sizeof(be64));
		return net::sockets::networkToHost64(be64);
	}
	// ��ȡint32����ƫ��
	int32_t peekInt32() const
	{
		assert(readableBytes() >= sizeof(int32_t));
		int32_t be32 = 0;
		::memcpy(&be32, peek(), sizeof(be32));
		return net::sockets::networkToHost32(be32);
	}
	// ��ȡint16����ƫ��
	int16_t peekInt16() const
	{
		assert(readableBytes() >= sizeof(int16_t));
		int16_t be16 = 0;
		::memcpy(&be16, peek(), sizeof(be16));
		return net::sockets::networkToHost16(be16);
	}
	// ��ȡint8����ƫ��
	int8_t peekInt8() const
	{
		assert(readableBytes() >= sizeof(int8_t));
		int8_t x = *peek();
		return x;
	}
	// д�������ֽ���int64����ǰԤ��λ��
	void prependInt64(int64_t x)
	{
		int64_t be64 = net::sockets::hostToNetwork64(x);
		prepend(&be64, sizeof(be64));
	}
	// д�������ֽ���int32����ǰԤ��λ��
	void prependInt32(int32_t x)
	{
		int32_t be32 = net::sockets::hostToNetwork32(x);
		prepend(&be32, sizeof(be32));
	}
	// д�������ֽ���int16����ǰԤ��λ��
	void prependInt16(int16_t x)
	{
		int16_t be16 = net::sockets::hostToNetwork16(x);
		prepend(&be16, sizeof(be16));
	}
	// д�������ֽ���int8����ǰԤ��λ��
	void prependInt8(int8_t x)
	{
		prepend(&x, sizeof(x));
	}
	// д��ָ�����ȵ����ݵ���ǰԤ��λ�ã���m_readerIndex����д��
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
	// �ڲ�buffer��capacity
	size_t internalCapacity() const
	{
		return m_buffer.capacity();
	}
	// ��socket�ж�ȡ����
	sz_ssize_t readvFd(sockets::sz_sock fd, int* savedErrno);
	sz_ssize_t readFd(sockets::sz_sock fd, int* savedErrno);
	sz_ssize_t recvFrom(sockets::sz_sock fd, struct sockaddr* addr, int* savedErrno);

private:
	// buffer������ͷ
	char* begin()
	{
		return &*m_buffer.begin();
	}
	const char* begin() const
	{
		return &*m_buffer.begin();
	}
	// ����
	void wrapBuffer()
	{
		if (m_readerIndex == m_nCheapPrepend)
		{
			return;
		}

		// �ɶ����ݳ���
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
	// ���ƻ�������������
	void makeSpace(size_t len)
	{
		if (writableBytes() + prependableBytes() < len + m_nCheapPrepend)
		{
			// ������ȷʵ������
			// FIXME: move readable data
			m_buffer.resize(m_writerIndex + len);
			m_wrapSize = m_buffer.capacity() * 3 / 4;
		}
		else
		{
			// ����
			assert(m_nCheapPrepend < m_readerIndex);
			wrapBuffer();
		}
	}

private:
	// Ԥ������
	const size_t m_nCheapPrepend;
	// ������
	std::vector<char> m_buffer;
	// �ɶ�λ����ʼ�±�
	size_t m_readerIndex;
	// ��дλ����ʼ�±�
	size_t m_writerIndex;
	// ���ƴ�С
	size_t m_wrapSize;
	// ���з�
	static const char kCRLF[];
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_DS_BUFFER_H_
