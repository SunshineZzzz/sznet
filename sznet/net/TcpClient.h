// Comment: TCP�ͻ���ʵ��

#ifndef _SZNET_NET_TCPCLIENT_H_
#define _SZNET_NET_TCPCLIENT_H_

#include "../thread/Mutex.h"
#include "TcpConnection.h"

namespace sznet
{
namespace net
{

// ��������ʵ��
class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

// TCP�ͻ���
class TcpClient : NonCopyable
{
public:
	// TcpClient(EventLoop* loop);
	// TcpClient(EventLoop* loop, const string& host, uint16_t port);
	TcpClient(EventLoop* loop, const InetAddress& serverAddr, const string& nameArg);
	// force out-line dtor, for std::unique_ptr members.
	~TcpClient();

	// ��������
	void connect();
	// ��رգ��ر�д��
	void disconnect();
	// ֹͣ����
	void stop();
	// ����TCP����
	TcpConnectionPtr connection() const
	{
		MutexLockGuard lock(m_mutex);
		return m_connection;
	}
	// ����loop
	EventLoop* getLoop() const 
	{ 
		return m_loop; 
	}
	// �Ƿ��������
	bool retry() const
	{ 
		return m_retry; 
	}
	// ��������
	void enableRetry() 
	{ 
		m_retry = true; 
	}
	// �����ӵ�����
	const string& name() const
	{ 
		return m_name; 
	}
	// �����ⲿ���ӽ���/�Ͽ��ص�����
	/// Set connection callback.
	/// Not thread safe.
	void setConnectionCallback(ConnectionCallback cb)
	{ 
		m_connectionCallback = std::move(cb); 
	}
	// �����ⲿ�����յ���Ϣ�Ļص�������
	/// Set message callback.
	/// Not thread safe.
	void setMessageCallback(MessageCallback cb)
	{ 
		m_messageCallback = std::move(cb); 
	}
	// ���÷��ͻ�������ջص�����
	/// Set write complete callback.
	/// Not thread safe.
	void setWriteCompleteCallback(WriteCompleteCallback cb)
	{ 
		m_writeCompleteCallback = std::move(cb); 
	}

private:
	// ���ӳɹ�
	/// Not thread safe, but in loop
	void newConnection(sockets::sz_sock sockfd);
	// ���ӶϿ�
	/// Not thread safe, but in loop
	void removeConnection(const TcpConnectionPtr& conn);

	// ���Ӱ󶨵�loop
	EventLoop* m_loop;
	// avoid revealing Connector
	// ��������
	ConnectorPtr m_connector; 
	// ���m_nextConnId��������������
	const string m_name;
	// �ⲿ���ӽ���/�Ͽ��ص�����
	ConnectionCallback m_connectionCallback;
	// �ⲿ�����յ���Ϣ�Ļص�������
	MessageCallback m_messageCallback;
	// ���ͻ�������ջص�����
	WriteCompleteCallback m_writeCompleteCallback;
	// Ĭ��false���ֶ����� enableRetry ����Ϊtrue
	// atomic
	bool m_retry;
	// Ĭ��true�������Ͽ������� false
	// atomic
	bool m_connect;
	// ���ӳɹ�����ID
	// always in loop thread
	int m_nextConnId;
	// ������
	mutable MutexLock m_mutex;
	// һ�� shared_ptr ����ʵ��ɱ�����߳�ͬʱ��ȡ
	// ���� shared_ptr ����ʵ����Ա������߳�ͬʱд�룬����������д����
	// ���Ҫ�Ӷ���̶߳�дͬһ�� shared_ptr ������ô��Ҫ����
	// tcp����
	TcpConnectionPtr m_connection;
};

} // namespace net

} // namespace sznet

#endif // _SZNET_NET_TCPCLIENT_H_
