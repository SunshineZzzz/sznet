// Comment: KcpWithTcp�ͻ���ʵ��

#ifndef _SZNET_NET_KCPWITHTCPCLIENT_H_
#define _SZNET_NET_KCPWITHTCPCLIENT_H_

#include "../thread/Mutex.h"
#include "../time/TimerId.h"
#include "TcpConnection.h"
#include "KcpConnection.h"
#include "Codec.h"
#include "InetAddress.h"
#include "Socket.h"
#include "EventLoop.h"
#include "Channel.h"

#include <atomic>

namespace sznet
{

namespace net
{

// ��������ʵ��
class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

// KCP�ͻ���
class KcpWithTcpClient : NonCopyable
{
public:
	KcpWithTcpClient(EventLoop* loop, const InetAddress& tcpServerAddr, const string& nameArg);
	// force out-line dtor, for std::unique_ptr members.
	~KcpWithTcpClient();

	// ����TCP����
	void connectTcp();
	// �ر�TCP����
	void disconnectTcp();
	// ֹͣTCP����
	void stopTcp();
	// KCP�Ƿ�����
	bool kcpConnected() const
	{
		MutexLockGuard lock(m_mutex);
		if (m_kcpConnection)
		{
			return m_kcpConnection->connected();
		}
		return false;
	}
	// ����KCP����
	KcpConnectionPtr kcpConnection() const
	{
		MutexLockGuard lock(m_mutex);
		return m_kcpConnection;
	}
	// ����loop
	EventLoop* getLoop() const
	{ 
		return m_loop; 
	}
	// �Ƿ��������
	bool tcpRetry() const
	{ 
		return m_tcpRetry; 
	}
	// ��������
	void enableTcpRetry() 
	{ 
		m_tcpRetry = true;
	}
	// �����ӵ�����
	const string& name() const
	{ 
		return m_name; 
	}
	// ����KCP���ӽ���/�Ͽ��ص�����
	/// Set connection callback.
	/// Not thread safe.
	void setKcpConnectionCallback(KcpConnectionCallback cb)
	{ 
		m_kcpConnectionCallback = std::move(cb); 
	}
	// ����KCP�����յ���Ϣ�Ļص�������
	/// Set message callback.
	/// Not thread safe.
	void setKcpMessageCallback(KcpMessageCallback cb)
	{ 
		m_kcpMessageCallback = std::move(cb); 
	}
	// ����KCP���ͻ�������ջص�����
	/// Set write complete callback.
	/// Not thread safe.
	void setKcpWriteCompleteCallback(KcpWriteCompleteCallback cb)
	{ 
		m_kcpWriteCompleteCallback = std::move(cb); 
	}

private:
	// TCP���ӳɹ��ص�
	/// Not thread safe, but in loop
	void newTcpConnection(sockets::sz_sock sockfd);
	// TCP���ӶϿ��ص�
	/// Not thread safe, but in loop
	void removeTcpConnection(const TcpConnectionPtr& tcpConn);
	// KCP���ӶϿ��ص�
	void removeKcpConnection(const KcpConnectionPtr& kcpConn);
	// TCP��Ϣ�ص�
	void handleTcpMessage(const TcpConnectionPtr& tcpConn, const string& message, Timestamp receiveTime);

	// ���Ӱ󶨵�loop
	EventLoop* m_loop;
	// avoid revealing Connector
	// ��������
	ConnectorPtr m_tcpConnector; 
	// ���m_nextConnId��������������
	const string m_name;
	// KCP���ӽ���/�Ͽ��ص�����
	KcpConnectionCallback m_kcpConnectionCallback;
	// KCP�����յ���Ϣ�Ļص�������
	KcpMessageCallback m_kcpMessageCallback;
	// KCP���ͻ�������ջص�����
	KcpWriteCompleteCallback m_kcpWriteCompleteCallback;
	// Ĭ��false���ֶ����� enableRetry ����Ϊtrue
	std::atomic<bool> m_tcpRetry;
	// Ĭ��true�������Ͽ������� false
	// �������ϵ��Ժ�������Ϊtrue�Ż���� m_connector->restart()
	std::atomic<bool> m_tcpConnect;
	// ���ӳɹ�����ID
	// always in loop thread
	uint32_t m_nextConnId;
	// ������
	mutable MutexLock m_mutex;
	// һ�� shared_ptr ����ʵ��ɱ�����߳�ͬʱ��ȡ
	// ���� shared_ptr ����ʵ����Ա������߳�ͬʱд�룬����������д����
	// ���Ҫ�Ӷ���̶߳�дͬһ�� shared_ptr ������ô��Ҫ����
	// TCP����
	TcpConnectionPtr m_tcpConnection;
	// 4�ֽڱ������
	LengthHeaderCodec<> m_tcpCodec;
	// UDP ����Addr
	// ��ʼ����ʱ���õ���TCP�ļ�����ַ
	// �õ�KCP�˿��Ժ��ٽ����޸�
	InetAddress m_udpListenAddr;
	// KCP����
	KcpConnectionPtr m_kcpConnection;
};

} // namespace net

} // namespace sznet

#endif // _SZNET_NET_KCPWITHTCPCLIENT_H_
