// Comment: ���ӽ�����ʵ��

#ifndef _SZNET_NET_ACCEPTOR_H_
#define _SZNET_NET_ACCEPTOR_H_

#include "Socket.h"
#include "Channel.h"

#include <functional>

namespace sznet
{

namespace net
{

// IO�¼�ѭ��
class EventLoop;
// �����ַ
class InetAddress;

// ���ӽ�����
class Acceptor : NonCopyable
{
public:
	// �����ӻص���������
	typedef std::function<void(sockets::sz_sock sockfd, const InetAddress&)> NewConnectionCallback;

	Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
	~Acceptor();

	// ���������ӵĻص�����
	void setNewConnectionCallback(const NewConnectionCallback& cb)
	{
		m_newConnectionCallback = cb;
	}
	// �Ƿ��ڼ���
	bool listenning() const 
	{ 
		return m_listenning; 
	}
	// ��������
	void listen();

private:
	// �ɶ��¼�������������
	void handleRead();

	// ������IO�¼�ѭ��
	EventLoop* m_loop;
	// ����socket
	Socket m_acceptSocket;
	// ��Ӧ���¼��ַ�
	Channel m_acceptChannel;
	// �û�����������ӻص�����
	NewConnectionCallback m_newConnectionCallback;
	// �Ƿ����ڼ���״̬
	bool m_listenning;
	// m_idleFd���÷�ֹbusy loop��
	// �����ʱ�������������ﵽ�����ޣ���accept��������ܻ�һֱ�������µ����ӵ�����ܣ�
	// sznet�õ���epoll��LTģʽʱ����ô�����Ϊ�����Ӵﵽ���ļ������������ޣ�
	// ��ʱû�пɹ��㱣���������׽������������ļ����ˣ�
	// ��ô���������Ӿͻ�һֱ����accept�����У����Ǻ����Ӧ�Ŀɶ��¼��ͻ�
	// һֱ�������¼�(��Ϊ��һֱ������Ҳû�취������)����ʱ�ͻ�������ǳ�˵��busy loop
	sockets::sz_sock m_idleFd;
};

} // namespace net

} // namespace sznet

#endif // _SZNET_NET_ACCEPTOR_H_
