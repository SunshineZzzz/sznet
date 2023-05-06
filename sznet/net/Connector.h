// Comment: ��������ʵ��

#ifndef _SZNET_NET_CONNECTOR_H_
#define _SZNET_NET_CONNECTOR_H_

#include "../base/NonCopyable.h" 
#include "InetAddress.h"

#include <functional>
#include <memory>

namespace sznet
{

namespace net
{

// �¼��ַ�
class Channel;
// IO�¼�ѭ��
class EventLoop;

// �������������������ӣ�������������
class Connector : NonCopyable, public std::enable_shared_from_this<Connector>
{
public:
	// ���ӳɹ��Ļص���������
	typedef std::function<void(sockets::sz_sock sockfd)> NewConnectionCallback;

	Connector(EventLoop* loop, const InetAddress& serverAddr);
	~Connector();

	// �������ӳɹ��Ļص�����
	void setNewConnectionCallback(const NewConnectionCallback& cb)
	{
		m_newConnectionCallback = cb;
	}
	// ��������
	// can be called in any thread
	void start();
	// ���·�������
	// must be called in loop thread
	void restart();
	// ֹͣ����
	// can be called in any thread
	void stop();
	// ����server�ĵ�ַ
	const InetAddress& serverAddress() const 
	{ 
		return m_serverAddr; 
	}

private:
	// ״̬
	enum States 
	{ 
		// �Ͽ�����
		kDisconnected, 
		// ��������
		kConnecting, 
		// �Ѿ�����
		kConnected 
	};
	// ��������ӳ�
	static const int kMaxRetryDelayMs = 30 * 1000;
	// ���������ӳ�
	static const int kInitRetryDelayMs = 500;

	// ��������״̬
	void setState(States s) 
	{ 
		m_state = s;
	}
	// m_loop�����߳��н�������
	void startInLoop();
	// ֹͣ��ǰ�����ӳ��ԣ����·���������
	void stopInLoop();
	// ��������
	void connect();
	// ��������
	void connecting(sockets::sz_sock sockfd);
	// ������connect��д�����ǲ�һ����ʾ�Ѿ��������� ����Ҫ��ȡerr���ж�
	void handleWrite();
	// ����
	void handleError();
	// ������������
	void retry(sockets::sz_sock sockfd);
	// ����¼� && �Ƴ�m_channel
	sockets::sz_sock removeAndResetChannel();
	// reset��m_channelΪ��
	void resetChannel();

	// ������EventLoop
	EventLoop* m_loop;
	// server��ַ
	InetAddress m_serverAddr;
	// �Ƿ�����������
	// atomic
	bool m_connect;
	// ����״̬
	// FIXME: use atomic variable
	States m_state;
	// �¼��ַ�
	std::unique_ptr<Channel> m_channel;
	// ���ӳɹ���Ļص�����
	NewConnectionCallback m_newConnectionCallback;
	// �����ӳ�
	int m_retryDelayMs;
};

} // namespace net

} // namespace sznet

#endif // _SZNET_NET_CONNECTOR_H_
