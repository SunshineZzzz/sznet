// Comment: 4�ֽڱ������ʵ��

#ifndef _SZNET_NET_LENHTHHEADERCODEC_H_
#define _SZNET_NET_LENHTHHEADERCODEC_H_

#include "../log/Logging.h"
#include "Buffer.h"
#include "Endian.h"
#include "TcpConnection.h"

// 4�ֽڱ������
class LengthHeaderCodec : sznet::NonCopyable
{
public:
	// �ְ��ص���������
	typedef std::function<void(const sznet::net::TcpConnectionPtr&, const sznet::string&, sznet::Timestamp)> StringMessageCallback;

	explicit LengthHeaderCodec(const StringMessageCallback& cb) :
		m_messageCallback(cb)
	{
	}

	// �ְ�
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
	// ���
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
	// �ְ��ص�����
	StringMessageCallback m_messageCallback;
	// ͷ�ĳ���
	const static size_t kHeaderLen = sizeof(int32_t);
};

#endif // _SZNET_NET_LENHTHHEADERCODEC_H_
