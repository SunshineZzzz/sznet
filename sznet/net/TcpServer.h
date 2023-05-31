// Comment: TCP������ʵ��

#ifndef _SZNET_NET_TCPSERVER_H_
#define _SZNET_NET_TCPSERVER_H_

#include "../NetCmn.h"
#include "TcpConnection.h"

#include <atomic>
#include <map>

namespace sznet
{

namespace net
{

// ���ӽ�����
class Acceptor;
// �¼�IOѭ��
class EventLoop;
// �¼�IOѭ���̳߳�
class EventLoopThreadPool;

// TCP��������֧�ֵ��̺߳��̳߳�
class TcpServer : NonCopyable
{
public:
	// �߳�IO�¼�ѭ��ǰ�Ļص���������
	typedef std::function<void(EventLoop*)> ThreadInitCallback;
	// ��ʼ��m_acceptor��������
	enum Option
	{
		// �����ö˿�
		kNoReusePort,
		// ���ö˿�
		kReusePort,
	};

	TcpServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg, Option option = kNoReusePort);
	~TcpServer();

	// ����ip&port�ַ���
	const string& ipPort() const 
	{ 
		return m_ipPort; 
	}
	// ��ȡ����������
	const string& name() const 
	{ 
		return m_name; 
	}
	// ��ȡ����loop
	EventLoop* getLoop() const 
	{ 
		return m_loop; 
	}
	// ����server����Ҫ���ж��ٸ�loop�߳�
	void setThreadNum(int numThreads);
	// ����IO�¼�ѭ��ǰ�Ļص�����
	void setThreadInitCallback(const ThreadInitCallback& cb)
	{
		m_threadInitCallback = cb;
	}
	// ��ȡ�̳߳�IO�¼�ѭ��
	// valid after calling start()
	std::shared_ptr<EventLoopThreadPool> threadPool()
	{
		return m_threadPool;
	}
	// ����Server�������ͻ�����
	// ���Զ�ε��ã��̰߳�ȫ
	void start();
	// �����ⲿ����/�Ͽ�֪ͨ�ص�����
	/// Set connection callback.
	/// Not thread safe.
	void setConnectionCallback(const ConnectionCallback& cb)
	{
		m_connectionCallback = cb;
	}
	// �ⲿ�����յ���Ϣ�Ļص�������
	/// Set message callback.
	/// Not thread safe.
	void setMessageCallback(const MessageCallback& cb)
	{
		m_messageCallback = cb;
	}
	// �������ݷ�����ɵĻص�������ֱ�ӷ��ͳɹ�/���ͻ������������
	/// Set write complete callback.
	/// Not thread safe.
	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{
		m_writeCompleteCallback = cb;
	}

private:
	// ����������
	// ֻ����accept io loop�ж����ã����̰߳�ȫ
	void newConnection(sockets::sz_sock sockfd, const InetAddress& peerAddr);
	// �Ƴ�ָ��������
	// �̰߳�ȫ
	void removeConnection(const TcpConnectionPtr& conn);
	// �Ƴ�ָ������
	// ֻ����accept io loop�ж����ã����̰߳�ȫ
	void removeConnectionInLoop(const TcpConnectionPtr& conn);
	// ���ӹ���������
	typedef std::map<uint32_t, TcpConnectionPtr> ConnectionMap;

	// ���������ӵ��̣߳��������ӻ����̳߳ط��ص�subReactor EventLoop��ִ��IO
	EventLoop* m_loop;
	// ������ַ�˿��ַ���
	const string m_ipPort;
	// ����������
	const string m_name;
	// ���ӽ�������ʹ�ø������������������ӣ���ͨ��������׽��������������sockfd
	std::unique_ptr<Acceptor> m_acceptor;
	// �̳߳�IO�¼�ѭ��
	std::shared_ptr<EventLoopThreadPool> m_threadPool;
	// TCP����/�Ͽ�֪ͨ�ص�����
	ConnectionCallback m_connectionCallback;
	// TCP�����յ���Ϣ�Ļص�������
	MessageCallback m_messageCallback;
	// TCP���ݷ�����ɵĻص�������ֱ�ӷ��ͳɹ�/���ͻ������������
	WriteCompleteCallback m_writeCompleteCallback;
	// IO�¼�ѭ��ǰ�Ļص�����
	ThreadInitCallback m_threadInitCallback;
	// ����ֻ����һ�η�����
	std::atomic<int> m_started;
	// ��������ID
	uint32_t m_nextConnId;
	// ���ӹ�����
	// �������� <-> ���Ӷ��������ָ��
	ConnectionMap m_connections;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_TCPSERVER_H_
