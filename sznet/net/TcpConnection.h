// Comment: TCP����ʵ��

#ifndef _SZNET_NET_TCPCONNECTION_H_
#define _SZNET_NET_TCPCONNECTION_H_

#include "../NetCmn.h"
#include "../base/NonCopyable.h"
#include "../string/StringPiece.h"
#include "../base/Callbacks.h"
#include "../time/Timestamp.h"
#include "Buffer.h"
#include "InetAddress.h"

#include <memory>

namespace sznet
{

namespace net
{

// �¼��ַ�
class Channel;
// �¼�ѭ��
class EventLoop;
// socket��װ
class Socket;

// TCP����(�Ѿ����Ӻã������������Ǳ���)��������ģ��
class TcpConnection : NonCopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
	// ��������״̬
	struct NetStatInfo
	{
		// �յ������ֽ�
		uint64_t m_recvBytes;
		// ���������ֽ�
		uint64_t m_sendBytes;

		NetStatInfo():
			m_recvBytes(0),
			m_sendBytes(0)
		{
		}
	};

public:
	TcpConnection(EventLoop* loop, const string& name, const uint32_t id, sockets::sz_sock sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
	~TcpConnection();

	// ��ȡTCP����������IO�¼�ѭ��
	EventLoop* getLoop() const 
	{ 
		return m_loop; 
	}
	// ��ȡ��������
	const string& name() const 
	{ 
		return m_name; 
	}
	// ��ȡ����ID
	const uint32_t id() const
	{
		return m_id;
	}
	// ��ȡ���ӵı��ص�ַ
	const InetAddress& localAddress() const 
	{ 
		return m_localAddr; 
	}
	// ��ȡ���ӵ�Զ�˵�ַ
	const InetAddress& peerAddress() const 
	{ 
		return m_peerAddr; 
	}
	// �Ƿ�����
	bool connected() const 
	{ 
		return m_state == kConnected;
	}
	// �Ƿ�Ͽ�����
	bool disconnected() const 
	{ 
		return m_state == kDisconnected; 
	}
	// �������� ���� д�뷢�ͻ�����
	void send(const void* message, int len);
	void send(const StringPiece& message);
	void send(Buffer* message);
	// ��رգ��ر�д��
	// NOT thread safe, no simultaneous calling
	void shutdown(); 
	// �ر�����
	void forceClose();
	// �ӳٹر�����
	void forceCloseWithDelay(double seconds);
	// �Ƿ���Nagle�㷨��true��ʾ�ر�
	void setTcpNoDelay(bool on);
	// ע��ɶ��¼�
	void startRead();
	// ȡ���ɶ��¼�
	void stopRead();
	// �Ƿ�ע��ɶ��¼�
	// NOT thread safe, may race with start/stopReadInLoop
	bool isReading() const 
	{ 
		return m_reading; 
	}; 
	// �ⲿ���ӽ���/�Ͽ��ص�����
	void setConnectionCallback(const ConnectionCallback& cb)
	{
		m_connectionCallback = cb;
	}
	// �ⲿ�����յ���Ϣ�Ļص�������
	void setMessageCallback(const MessageCallback& cb)
	{
		m_messageCallback = cb;
	}
	// ���÷��͵�����ȫ��������ϵĻص�����
	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{
		m_writeCompleteCallback = cb;
	}
	// ���ø�ˮλ�ߣ���Ӧ�ĸ�ˮλ�߻ص�����
	void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
	{
		m_highWaterMarkCallback = cb; 
		m_highWaterMark = highWaterMark;
	}
	// ���ؽ��ջ�����
	Buffer* inputBuffer()
	{
		return &m_inputBuffer;
	}
	// ���ط��ͻ�����
	Buffer* outputBuffer()
	{
		return &m_outputBuffer;
	}
	// �����ڲ����ӶϿ��ص�����
	void setCloseCallback(const CloseCallback& cb)
	{
		m_closeCallback = cb;
	}
	// ���ӽ�������TcpServer�н������Ӻ����ô˺�����ע����¼�
	// called when TcpServer accepts a new connection
	// should be called only once
	void connectEstablished();
	// �ر����ӣ��ṩ��TcpServerʹ�ã��Ƴ������¼�
	// called when TcpServer has removed me from its map
	// should be called only once
	void connectDestroyed();
	// ͳ���������ݣ���̬Ӱ��Ӧ�ò���
	void calcNetData(NetStatInfo& deltaInfo, double& dTick)
	{
		auto tick = Timestamp::now();
		dTick = timeDifference(tick, m_calcNetLastTime);
		m_calcNetLastTime = tick;

		uint64_t deltaRecv = m_netStatInfo.m_recvBytes - m_prevNetStatInfo.m_recvBytes;
		m_prevNetStatInfo.m_recvBytes = m_netStatInfo.m_recvBytes;
		
		uint64_t deltaSend = m_netStatInfo.m_sendBytes - m_prevNetStatInfo.m_sendBytes;
		m_prevNetStatInfo.m_sendBytes = m_netStatInfo.m_sendBytes;

		deltaInfo.m_recvBytes = static_cast<uint64_t>(deltaRecv / dTick);
		deltaInfo.m_sendBytes = static_cast<uint64_t>(deltaSend / dTick);
	}
	// shrink�շ�������
	void shrinkRSBuffer(size_t sendSize, size_t recvSize);
	// m_loop�����߳��йر����ӣ��ر�д��
	void forceCloseInLoop();

private:
	// ����״̬
	enum StateE 
	{ 
		// �ײ����ӶϿ�
		kDisconnected, 
		// �ײ����ӳɹ���listen���ڵ�loop�߳�
		kConnecting, 
		// �߼������ӳɹ�������loop�̻߳ص�connectEstablished����
		kConnected, 
		// �������ڶϿ����ӣ����ڹر�д��
		kDisconnecting 
	};
	// socket�ɶ��¼�
	void handleRead(Timestamp receiveTime);
	// socket��д�¼�
	void handleWrite();
	// socket�ر��¼�
	void handleClose();
	// socket�����¼�
	void handleError();
	// m_loop�����߳���send
	void sendInLoop(const StringPiece& message);
	void sendInLoop(const void* message, size_t len);
	// m_loop�����߳��а�رգ��ر�д��
	void shutdownInLoop();
	// ����״̬
	void setState(StateE s) 
	{ 
		m_state = s; 
	}
	// ��tcp����״̬ת�����ַ�����ʽ
	const char* stateToString() const;
	//  m_loop�����߳����¼������ע�ɶ��¼�
	void startReadInLoop();
	// m_loop�����߳����¼�����ȡ���ɶ��¼�
	void stopReadInLoop();
	// m_loop�����߳���shrink�շ�������
	void shrinkRSBufferInLoop(size_t sendSize, size_t recvSize);

	// ����IO�¼�ѭ��
	EventLoop* m_loop;
	// ��������
	const string m_name;
	// ����ID
	uint32_t m_id;
	// ����״̬
	StateE m_state;
	// �Ƿ�ע��ɶ��¼�
	bool m_reading;
	// �Ѿ������õ�socket
	std::unique_ptr<Socket> m_socket;
	// �¼�
	std::unique_ptr<Channel> m_channel;
	// ���ط�������ַ
	const InetAddress m_localAddr;
	// �Է��ͻ��˵�ַ
	const InetAddress m_peerAddr;
	// TCP���ӽ���/�Ͽ��ص�����
	ConnectionCallback m_connectionCallback;
	// TCP�����յ���Ϣ�Ļص�������
	MessageCallback m_messageCallback;
	// TCP���ݷ�����ɵĻص�������ֱ�ӷ��ͳɹ�/���ͻ������������
	WriteCompleteCallback m_writeCompleteCallback;
	// TCP���͸�ˮλ�߻ص������������ش���һ��
	HighWaterMarkCallback m_highWaterMarkCallback;
	// TCP���ӶϿ��ص�����
	CloseCallback m_closeCallback;
	// ���͸�ˮλ��
	size_t m_highWaterMark;
	// ���ջ�����
	Buffer m_inputBuffer;
	// ���ͻ�����
	Buffer m_outputBuffer;
	// ��ǰ��������
	NetStatInfo m_netStatInfo;
	// ��һ����������
	NetStatInfo m_prevNetStatInfo;
	// ��һ��ͳ�������ʱ��
	Timestamp m_calcNetLastTime;
};

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_TCPCONNECTION_H_
