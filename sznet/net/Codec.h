// Comment: 4字节编解码器实现

#ifndef _SZNET_NET_LENHTHHEADERCODEC_H_
#define _SZNET_NET_LENHTHHEADERCODEC_H_

#include "../log/Logging.h"
#include "Buffer.h"
#include "Endian.h"
#include "TcpConnection.h"

// 4字节编解码器
class LengthHeaderCodec : sznet::NonCopyable
{
public:
	// 分包回调函数类型
	typedef std::function<void(const sznet::net::TcpConnectionPtr&, const sznet::string&, sznet::Timestamp)> StringMessageCallback;

	explicit LengthHeaderCodec(const StringMessageCallback& cb) :
		m_messageCallback(cb)
	{
	}

	// 分包
	void onMessage(const sznet::net::TcpConnectionPtr& conn, sznet::net::Buffer* buf, sznet::Timestamp receiveTime)
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
				sznet::string message(buf->peek(), len);
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
	// FIXME: TcpConnectionPtr
	void send(sznet::net::TcpConnection* conn, const sznet::StringPiece& message)
	{
		sznet::net::Buffer buf;
		buf.append(message.data(), message.size());
		int32_t len = static_cast<int32_t>(message.size());
		buf.prepend(&len, sizeof(len));
		conn->send(&buf);
	}
	void send(sznet::net::TcpConnection* conn, const void* pData, size_t size)
	{
		sznet::net::Buffer buf;
		buf.append(pData, sznet::implicit_cast<size_t>(size));
		int32_t len = static_cast<int32_t>(size);
		buf.prepend(&len, sizeof(len));
		conn->send(&buf);
	}
	void send(sznet::net::TcpConnection* conn, sznet::net::Buffer& buf, size_t size)
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

#endif // _SZNET_NET_LENHTHHEADERCODEC_H_
