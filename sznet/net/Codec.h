// Comment: 4�ֽڱ������ʵ��

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

// 4�ֽڱ������
template <typename TSPTR = TcpConnectionPtr>
class LengthHeaderCodec : NonCopyable
{
public:
	typedef typename TSPTR::element_type TPTR;
	// �ְ��ص���������
	typedef std::function<void(const TSPTR&, const string&, Timestamp)> StringMessageCallback;

	explicit LengthHeaderCodec(const StringMessageCallback& cb) :
		m_messageCallback(cb)
	{
	}

	// �ְ�
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
	// ���
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
	// �ְ��ص�����
	StringMessageCallback m_messageCallback;
	// ͷ�ĳ���
	const static size_t kHeaderLen = sizeof(int32_t);
};

}

}
#endif // _SZNET_NET_LENHTHHEADERCODEC_H_
