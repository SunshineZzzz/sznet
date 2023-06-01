// Comment: KCP������

#ifndef _SZNET_NET_KCPCONNECTION_H_
#define _SZNET_NET_KCPCONNECTION_H_

#include "../NetCmn.h"
#include "../base/NonCopyable.h"
#include "../string/StringPiece.h"
#include "../base/Callbacks.h"
#include "../time/Timestamp.h"
#include "../../3rd/kcp/ikcp.h"
#include "../time/TimerId.h"
#include "Buffer.h"
#include "InetAddress.h"
#include "Socket.h"
#include "EventLoop.h"
#include "Channel.h"

#include <memory>

namespace sznet
{

namespace net
{

// KCP&TCP�¼�ѭ�� 
class KcpTcpEventLoop;

// KCP���ӣ���������ģ��
class KcpConnection : NonCopyable, public std::enable_shared_from_this<KcpConnection>
{
public:
	// KCPͷ�Ĵ�С
	static const int kKCP_OVERHEAD_SIZE = 24;
	// KCP����
	struct KcpSettings
	{
		// �Ƿ�����nodelayģʽ��0������
		uint8_t m_noDelay = 0;
		// KCP�ڲ�update�������λ��ms
		int m_interval = 10;
		// �����ش�ģʽ
		int m_resend = 0;
		// �Ƿ������أ�0�����ã�1����
		uint8_t m_noFlowControl = 0;
		// 1472�ֽڡ�������ݲ��ִ���1472�ֽڣ��ͻ���ַ�Ƭ����
		int m_mtu = (1500 - 2 - 8);
		// ���ʹ��ڣ���λ�ǰ�����λ��mtu
		// �����鳬��1024
		int m_sndWnd = 256;
		// ���մ��ڲ�С�ڷ��ʹ��ڣ���λ��mtu
		int m_rcvWnd = 512;
		// ��СRTO����λ��ms
		// ������TCP����KCP����RTOʱ������СRTO�����ƣ�����������RTOΪ40ms������Ĭ�ϵ�RTO��100ms��
		// Э��ֻ����100ms����ܼ�⵽����������ģʽ��Ϊ30ms�������ֶ����ĸ�ֵ
		int m_minrto = 30;
		// �Ƿ�����ģʽ��0�����ã�1����
		uint8_t m_isStream = 1;

		KcpSettings(uint8_t noDelay, int interval, int resend, uint8_t noFlowControl, int minrto):
			m_noDelay(noDelay),
			m_interval(interval),
			m_resend(resend),
			m_noFlowControl(noFlowControl),
			m_minrto(minrto),
			m_isStream(1),
			m_mtu(1500 - 2 - 8),
			m_sndWnd(256),
			m_rcvWnd(512)
		{
		}
	};
	// ��ȡKCP��ͨģʽ����
	static const KcpSettings getNormalKcpSet();
	// ��ȡKCP����ģʽ����
	static const KcpSettings getQuickKcpSet();

	// ��������״̬
	struct NetStatInfo
	{
		// �յ������ֽ�
		uint64_t m_recvBytes;
		// ���������ֽ�
		uint64_t m_sendBytes;

		NetStatInfo() :
			m_recvBytes(0),
			m_sendBytes(0)
		{
		}
	};

	// ����״̬
	enum StateE
	{
		// �Ͽ�����
		kDisconnected,
		// �ӳٹر���
		kDelayDisconnecting,
		// ���ڹر�
		kDisconnecting,
		// ��������
		kConnecting,
		// �Ѿ�����
		kConnected,
	};

public:
	KcpConnection(EventLoop* loop, sockets::sz_sock fd, int secretId, const string& name, const uint32_t id, bool isClient = true, int kcpMode = 1);
	~KcpConnection();

	// ��ȡKCP����������IO�¼�ѭ��
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
	// ��ȡ���ӵ�Զ�˵�ַ
	const InetAddress peerAddress() const
	{
		if (m_peerAddr)
		{
			return *m_peerAddr;
		}
		return InetAddress();
	}
	// ��ȡ���ӵı��ص�ַ
	const InetAddress localAddress() const
	{
		return InetAddress(sockets::sz_sock_getlocaladdr(m_udpSocket.fd()));
	}
	// �Ƿ�����
	bool connected() const
	{
		return m_state == kConnected;
	}
	// �Ƿ���������
	bool connecting() const
	{
		return m_state == kConnecting;
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
	// �ر�����
	void forceClose();
	// �ӳٹر�����
	void forceCloseWithDelay(double seconds);
	// KCP���ӽ���/�Ͽ��ص�����
	void setKcpConnectionCallback(const KcpConnectionCallback& cb)
	{
		m_kcpConnectionCallback = cb;
	}
	// KCP�����յ���Ϣ�Ļص�������
	void setKcpMessageCallback(const KcpMessageCallback& cb)
	{
		m_kcpMessageCallback = cb;
	}
	// KCP���÷��͵�����ȫ��������ϵĻص�����
	void setKcpWriteCompleteCallback(const KcpWriteCompleteCallback& cb)
	{
		m_kcpWriteCompleteCallback = cb;
	}
	// KCP���ø�ˮλ�ߣ���Ӧ�ĸ�ˮλ�߻ص�����
	void setKcpHighWaterMarkCallback(const KcpHighWaterMarkCallback& cb, size_t highWaterMark)
	{
		m_kcpHighWaterMarkCallback = cb;
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
	// KCP�����ڲ����ӶϿ��ص�����
	void setKcpCloseCallback(const KcpCloseCallback& cb)
	{
		m_kcpCloseCallback = cb;
	}
	// ���ӽ�������KcpTcpEventLoop�н������Ӻ����ô˺�����ע��Update
	// should be called only once
	void connectEstablished();
	// �ر����ӣ��ṩ��KcpTcpEventLoopʹ��
	// called when KcpTcpEventLoop has removed me from its map
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
	// �ⲿ�����Ƿ�����������Ӧloop�߳�
	void assertInLoopThread()
	{
		m_loop->assertInLoopThread();
	}
	// shrink�շ�������
	void shrinkRSBuffer(size_t sendSize, size_t recvSize);
	// ����KCP����
	// after construct
	void newKCP();
	// ����״̬
	void setState(StateE s)
	{
		m_state = s;
	}
	// ����Զ�˵�ַ
	void setPeerAddr(const InetAddress addr)
	{
		m_peerAddr.reset(new InetAddress(addr));
	}
	// ��ȡ��Կ
	int getSecretId() const
	{
		return m_secretId;
	}
	// ��ȡ����ID
	uint32_t getConvId() const
	{
		return m_id;
	}
	// ����kcp��Ϣ������Ժ���Խ����߼���
	void handleKcpMessage(Buffer& inputBuffer);
	// �ر�Kcp����
	void handleKcpClose();
	// m_loop�����߳��йر����ӣ��ر�д��
	void forceCloseInLoop();
	// UDP��ִ����У��
	void udpSendHandShakeVerify();
	// ����UDP��ִ����У���timer
	void udpSendHandShakeTimer();

private:
	// �����뷢һ�����ֻ�ִ
	static const double kEverySendVerify;
	// ����
	static const int kFirstKcpUpdateMilliInterval = 10;
	// kcp������Ϣ�ӿ�
	static int kcpOutput(const char* buf, int len, ikcpcb* kcp, void* user);
	// m_loop�����߳���send
	void sendInLoop(const StringPiece& message);
	void sendInLoop(const char* data, int len);
	// ��kcp����״̬ת�����ַ�����ʽ
	const char* stateToString() const;
	// m_loop�����߳���shrink�շ�������
	void shrinkRSBufferInLoop(size_t sendSize, size_t recvSize);
	// ����update�����������´�timer���ܶ���ʼ
	void kcpUpdateManual();
	// �ͻ��˲Ż��õ����
	void handleUdpMessage(Timestamp receiveTime);

	// ����IO�¼�ѭ��
	EventLoop* m_loop;
	// udp socket
	Socket m_udpSocket;
	// ��������
	const string m_name;
	// ����ID
	uint32_t m_id;
	// ����״̬
	StateE m_state;
	// �Է���ַ
	std::unique_ptr<InetAddress> m_peerAddr;
	// Kcp���ӽ���/�Ͽ��ص�����
	KcpConnectionCallback m_kcpConnectionCallback;
	// Kcp�����յ���Ϣ�Ļص�������
	KcpMessageCallback m_kcpMessageCallback;
	// Kcp���ݷ�����ɵĻص�������ֱ�ӷ��ͳɹ�/���ͻ������������
	KcpWriteCompleteCallback m_kcpWriteCompleteCallback;
	// Kcp���͸�ˮλ�߻ص������������ش���һ��
	KcpHighWaterMarkCallback m_kcpHighWaterMarkCallback;
	// KCP���ӶϿ��ص�����
	KcpCloseCallback m_kcpCloseCallback;
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
	// kcp�ײ����
	ikcpcb* m_kcp;
	// kcp update timer
	TimerId m_kcpUpdateTimerId;
	// �Ƿ���client
	const bool m_isClient;
	// ��Կ
	const int m_secretId;
	// kcp ����
	KcpSettings m_kcpSet;
	// �¼�
	std::unique_ptr<Channel> m_udpChannel;
	// ���ֳ�ʱ
	TimerId m_verifyTimerId;
};

inline const KcpConnection::KcpSettings KcpConnection::getNormalKcpSet()
{
	return KcpConnection::KcpSettings(0, 40, 0, 0, 50);
}

inline const KcpConnection::KcpSettings KcpConnection::getQuickKcpSet()
{
	return KcpConnection::KcpSettings(1, 10, 2, 1, 50);
}

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_KCPCONNECTION_H_
