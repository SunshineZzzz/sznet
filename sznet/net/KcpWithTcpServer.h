// Comment: Kcp Cooperate With Tcp Server

#ifndef _SZNET_NET_KCPWITHTCPSERVER_H_
#define _SZNET_NET_KCPWITHTCPSERVER_H_

#include "../NetCmn.h"
#include "KcpConnection.h"
#include "Codec.h"

#include <atomic>
#include <vector>
#include <map>
#include <random>

namespace sznet
{

namespace net
{

// ���ӽ�����
class Acceptor;
// KCP&TCP�¼�IOѭ��
class KcpTcpEventLoop;
// KCP&TCP�¼�IOѭ���̳߳�
class KcpTcpEventLoopThreadPool;

// ����TCP����ʹ��KCP���񣬾����뿴�������ӣ�
// https://github.com/skywind3000/kcp/wiki/Cooperate-With-Tcp-Server
class KcpWithTcpServer : NonCopyable
{
public:
	// �߳�IO�¼�ѭ��ǰ�Ļص���������
	typedef std::function<void(KcpTcpEventLoop*)> ThreadInitCallback;
	// ��ʼ��m_acceptor��������
	enum Option
	{
		// �����ö˿�
		kNoReusePort,
		// ���ö˿�
		kReusePort,
	};

	KcpWithTcpServer(KcpTcpEventLoop* loop, const InetAddress& listenAddr, const std::vector<InetAddress>& udpListenAddrs, const string& nameArg, Option option = kNoReusePort);
	~KcpWithTcpServer();

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
	KcpTcpEventLoop* getLoop() const
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
	std::shared_ptr<KcpTcpEventLoopThreadPool> threadPool()
	{
		return m_threadPool;
	}
	// ����Server�������ͻ�����
	// ���Զ�ε��ã��̰߳�ȫ
	void start();
	// ����KCP����/�Ͽ�֪ͨ�ص�����
	/// Set connection callback.
	/// Not thread safe.
	void setKcpConnectionCallback(const KcpConnectionCallback& cb)
	{
		m_kcpConnectionCallback = cb;
	}
	// KCP�����յ���Ϣ�Ļص�������
	/// Set message callback.
	/// Not thread safe.
	void setKcpMessageCallback(const KcpMessageCallback& cb)
	{
		m_kcpMessageCallback = cb;
	}
	// ����KCP���ݷ�����ɵĻص�������ֱ�ӷ��ͳɹ�/���ͻ������������
	/// Set write complete callback.
	/// Not thread safe.
	void setKcpWriteCompleteCallback(const KcpWriteCompleteCallback& cb)
	{
		m_kcpWriteCompleteCallback = cb;
	}

private:
	// ���ӹ���������
	typedef std::map<uint32_t, TcpConnectionPtr> ConnectionMap;

	// ����������
	// ֻ����accept io loop�ж����ã����̰߳�ȫ
	void newConnection(sockets::sz_sock sockfd, const InetAddress& peerAddr);
	// �Ƴ�ָ��������
	// �̰߳�ȫ
	void removeConnection(const TcpConnectionPtr& conn);
	// �Ƴ�ָ������
	// ֻ����accept io loop�ж����ã����̰߳�ȫ
	void removeConnectionInLoop(const TcpConnectionPtr& conn);
	// tcp�µ����� ���� �Ͽ�����
	void tcpConnection(const TcpConnectionPtr& conn, KcpTcpEventLoop* pKcpTcploop);
	// tcp��Ϣ�������ϲ����յ��κ���Ϣ��LengthHeaderCodec��Ҫ�����ʼ��
	void tcpMessage(const sznet::net::TcpConnectionPtr& conn, const sznet::string& message, sznet::Timestamp receiveTime);
	// ��Ҫ��������KCP�Ļص�
	void onThreadInitCallback(KcpTcpEventLoop* loop);

	// ���������ӵ��̣߳��������ӻ����̳߳ط��ص�subReactor EventLoop��ִ��IO
	KcpTcpEventLoop* m_loop;
	// ������ַ�˿��ַ���
	const string m_ipPort;
	// ����������
	const string m_name;
	// ���ӽ�������ʹ�ø������������������ӣ���ͨ��������׽��������������sockfd
	std::unique_ptr<Acceptor> m_acceptor;
	// �̳߳�IO�¼�ѭ��
	std::shared_ptr<KcpTcpEventLoopThreadPool> m_threadPool;
	// Kcp����/�Ͽ�֪ͨ�ص�����
	KcpConnectionCallback m_kcpConnectionCallback;
	// KCP�����յ���Ϣ�Ļص�������
	KcpMessageCallback m_kcpMessageCallback;
	// TCP���ݷ�����ɵĻص�������ֱ�ӷ��ͳɹ�/���ͻ������������
	KcpWriteCompleteCallback m_kcpWriteCompleteCallback;
	// IO�¼�ѭ��ǰ�Ļص�����
	ThreadInitCallback m_threadInitCallback;
	// ����ֻ����һ�η�����
	std::atomic<int> m_started;
	// ��������ID
	uint32_t m_nextConnId;
	// ���ӹ�����
	// ����ID <-> ���Ӷ��������ָ��
	ConnectionMap m_connections;
	// UDP�����ַ�����߳�����������һ��
	std::vector<InetAddress> m_udpListenAddrs;
	// 4�ֽڱ������
	LengthHeaderCodec<> m_tcpCodec;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_KCPWITHTCPSERVER_H_
