// Comment: 4字节编解码器实现

#ifndef _SZNET_NET_LENHTHHEADERCODEC_H_
#define _SZNET_NET_LENHTHHEADERCODEC_H_

#include "../log/Logging.h"
#include "Buffer.h"
#include "Endian.h"
#include "TcpConnection.h"

namespace sznet
{

namespace net
{

// 4字节编解码器
template <typename TSPTR = TcpConnectionPtr>
class LengthHeaderCodec : NonCopyable
{
public:
	typedef typename TSPTR::element_type TPTR;
	// 分包回调函数类型
	typedef std::function<void(const TSPTR&, const string&, Timestamp)> StringMessageCallback;

	explicit LengthHeaderCodec(const StringMessageCallback& cb) :
		m_messageCallback(cb)
	{
	}

	// 分包
	void onMessage(const TSPTR& conn, Buffer* buf, Timestamp receiveTime)
	{
		// kHeaderLen == 4
		while (buf->readableBytes() >= kHeaderLen)
		{
			// FIXME: use Buffer::peekInt32()
			const void* data = buf->peek();
			// SIGBUS
			const int32_t len = *static_cast<const int32_t*>(data);
			if (len < 0)
			{
				LOG_ERROR << "Invalid length " << len;
				conn->forceClose();
				break;
			}
			else if (buf->readableBytes() >= len + kHeaderLen)
			{
				buf->retrieve(kHeaderLen);
				string message(buf->peek(), len);
				m_messageCallback(conn, message, receiveTime);
				buf->retrieve(len);
			}
			else
			{
				break;
			}
		}
	}
	// 打包
	// FIXME: TSPTR
	void send(TPTR* conn, const StringPiece& message)
	{
		Buffer buf;
		buf.append(message.data(), message.size());
		int32_t len = static_cast<int32_t>(message.size());
		buf.prepend(&len, sizeof(len));
		conn->send(&buf);
	}
	void send(TPTR* conn, const void* pData, size_t size)
	{
		Buffer buf;
		buf.append(pData, implicit_cast<size_t>(size));
		int32_t len = static_cast<int32_t>(size);
		buf.prepend(&len, sizeof(len));
		conn->send(&buf);
	}
	void send(TPTR* conn, Buffer& buf, size_t size)
	{
		int32_t len = static_cast<int32_t>(size);
		buf.prepend(&len, sizeof(len));
		conn->send(&buf);
	}

private:
	// 分包回调函数
	StringMessageCallback m_messageCallback;
	// 头的长度
	const static size_t kHeaderLen = sizeof(int32_t);
};

}

}
#endif // _SZNET_NET_LENHTHHEADERCODEC_H_
