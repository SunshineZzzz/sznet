// Comment: KCP连接器

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

// KCP&TCP事件循环 
class KcpTcpEventLoop;

// KCP连接，生命周期模糊
class KcpConnection : NonCopyable, public std::enable_shared_from_this<KcpConnection>
{
public:
	// KCP头的大小
	static const int kKCP_OVERHEAD_SIZE = 24;
	// KCP配置
	struct KcpSettings
	{
		// 是否启用nodelay模式，0不启用
		uint8_t m_noDelay = 0;
		// KCP内部update间隔，单位是ms
		int m_interval = 10;
		// 快速重传模式
		int m_resend = 0;
		// 是否开启流控，0不启用，1启用
		uint8_t m_noFlowControl = 0;
		// 1472字节。如果数据部分大于1472字节，就会出现分片现象
		int m_mtu = (1500 - 2 - 8);
		// 发送窗口，单位是包，单位是mtu
		// 不建议超过1024
		int m_sndWnd = 256;
		// 接收窗口不小于发送窗口，单位是mtu
		int m_rcvWnd = 512;
		// 最小RTO，单位是ms
		// 不管是TCP还是KCP计算RTO时都有最小RTO的限制，即便计算出来RTO为40ms，由于默认的RTO是100ms，
		// 协议只有在100ms后才能检测到丢包，快速模式下为30ms，可以手动更改该值
		int m_minrto = 30;
		// 是否开启流模式，0不启用，1启用
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
	// 获取KCP普通模式配置
	static const KcpSettings getNormalKcpSet();
	// 获取KCP急速模式配置
	static const KcpSettings getQuickKcpSet();

	// 网络数据状态
	struct NetStatInfo
	{
		// 收到数据字节
		uint64_t m_recvBytes;
		// 发送数据字节
		uint64_t m_sendBytes;

		NetStatInfo() :
			m_recvBytes(0),
			m_sendBytes(0)
		{
		}
	};

	// 连接状态
	enum StateE
	{
		// 断开连接
		kDisconnected,
		// 延迟关闭中
		kDelayDisconnecting,
		// 正在关闭
		kDisconnecting,
		// 正在连接
		kConnecting,
		// 已经连接
		kConnected,
	};

public:
	KcpConnection(EventLoop* loop, sockets::sz_sock fd, int secretId, const string& name, const uint32_t id, bool isClient = true, int kcpMode = 1);
	~KcpConnection();

	// 获取KCP连接所属的IO事件循环
	EventLoop* getLoop() const
	{
		return m_loop;
	}
	// 获取连接名称
	const string& name() const
	{
		return m_name;
	}
	// 获取连接ID
	const uint32_t id() const
	{
		return m_id;
	}
	// 获取连接的远端地址
	const InetAddress peerAddress() const
	{
		if (m_peerAddr)
		{
			return *m_peerAddr;
		}
		return InetAddress();
	}
	// 获取连接的本地地址
	const InetAddress localAddress() const
	{
		return InetAddress(sockets::sz_sock_getlocaladdr(m_udpSocket.fd()));
	}
	// 是否连接
	bool connected() const
	{
		return m_state == kConnected;
	}
	// 是否正在连接
	bool connecting() const
	{
		return m_state == kConnecting;
	}
	// 是否断开连接
	bool disconnected() const
	{
		return m_state == kDisconnected;
	}
	// 发送数据 或者 写入发送缓冲区
	void send(const void* message, int len);
	void send(const StringPiece& message);
	void send(Buffer* message);
	// 关闭连接
	void forceClose();
	// 延迟关闭连接
	void forceCloseWithDelay(double seconds);
	// KCP连接建立/断开回调函数
	void setKcpConnectionCallback(const KcpConnectionCallback& cb)
	{
		m_kcpConnectionCallback = cb;
	}
	// KCP处理收到消息的回调调函数
	void setKcpMessageCallback(const KcpMessageCallback& cb)
	{
		m_kcpMessageCallback = cb;
	}
	// KCP设置发送的数据全部发送完毕的回调函数
	void setKcpWriteCompleteCallback(const KcpWriteCompleteCallback& cb)
	{
		m_kcpWriteCompleteCallback = cb;
	}
	// KCP设置高水位线，对应的高水位线回调函数
	void setKcpHighWaterMarkCallback(const KcpHighWaterMarkCallback& cb, size_t highWaterMark)
	{
		m_kcpHighWaterMarkCallback = cb;
		m_highWaterMark = highWaterMark;
	}
	// 返回接收缓冲区
	Buffer* inputBuffer()
	{
		return &m_inputBuffer;
	}
	// 返回发送缓冲区
	Buffer* outputBuffer()
	{
		return &m_outputBuffer;
	}
	// KCP设置内部连接断开回调函数
	void setKcpCloseCallback(const KcpCloseCallback& cb)
	{
		m_kcpCloseCallback = cb;
	}
	// 连接建立，在KcpTcpEventLoop中建立连接后会调用此函数，注册Update
	// should be called only once
	void connectEstablished();
	// 关闭连接，提供给KcpTcpEventLoop使用
	// called when KcpTcpEventLoop has removed me from its map
	// should be called only once
	void connectDestroyed();
	// 统计网络数据，竞态影响应该不大
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
	// 外部断言是否存在于自身对应loop线程
	void assertInLoopThread()
	{
		m_loop->assertInLoopThread();
	}
	// shrink收发缓冲区
	void shrinkRSBuffer(size_t sendSize, size_t recvSize);
	// 创建KCP对象
	// after construct
	void newKCP();
	// 设置状态
	void setState(StateE s)
	{
		m_state = s;
	}
	// 设置远端地址
	void setPeerAddr(const InetAddress addr)
	{
		m_peerAddr.reset(new InetAddress(addr));
	}
	// 获取密钥
	int getSecretId() const
	{
		return m_secretId;
	}
	// 获取连接ID
	uint32_t getConvId() const
	{
		return m_id;
	}
	// 处理kcp消息，解包以后可以交由逻辑层
	void handleKcpMessage(Buffer& inputBuffer);
	// 关闭Kcp连接
	void handleKcpClose();
	// m_loop所在线程中关闭连接，关闭写端
	void forceCloseInLoop();
	// UDP回执握手校验
	void udpSendHandShakeVerify();
	// 设置UDP回执握手校验的timer
	void udpSendHandShakeTimer();

private:
	// 多少秒发一次握手回执
	static const double kEverySendVerify;
	// 毫秒
	static const int kFirstKcpUpdateMilliInterval = 10;
	// kcp发送消息接口
	static int kcpOutput(const char* buf, int len, ikcpcb* kcp, void* user);
	// m_loop所在线程中send
	void sendInLoop(const StringPiece& message);
	void sendInLoop(const char* data, int len);
	// 把kcp连接状态转换成字符串形式
	const char* stateToString() const;
	// m_loop所在线程中shrink收发缓冲区
	void shrinkRSBufferInLoop(size_t sendSize, size_t recvSize);
	// 触发update，并且设置下次timer，周而复始
	void kcpUpdateManual();
	// 客户端才会用到这个
	void handleUdpMessage(Timestamp receiveTime);

	// 所属IO事件循环
	EventLoop* m_loop;
	// udp socket
	Socket m_udpSocket;
	// 连接名称
	const string m_name;
	// 连接ID
	uint32_t m_id;
	// 连接状态
	StateE m_state;
	// 对方地址
	std::unique_ptr<InetAddress> m_peerAddr;
	// Kcp连接建立/断开回调函数
	KcpConnectionCallback m_kcpConnectionCallback;
	// Kcp处理收到消息的回调调函数
	KcpMessageCallback m_kcpMessageCallback;
	// Kcp数据发送完成的回调函数，直接发送成功/发送缓冲区发送完毕
	KcpWriteCompleteCallback m_kcpWriteCompleteCallback;
	// Kcp发送高水位线回调函数，上升沿触发一次
	KcpHighWaterMarkCallback m_kcpHighWaterMarkCallback;
	// KCP连接断开回调函数
	KcpCloseCallback m_kcpCloseCallback;
	// 发送高水位线
	size_t m_highWaterMark;
	// 接收缓冲区
	Buffer m_inputBuffer;
	// 发送缓冲区
	Buffer m_outputBuffer;
	// 当前网络数据
	NetStatInfo m_netStatInfo;
	// 上一次网络数据
	NetStatInfo m_prevNetStatInfo;
	// 上一次统计网络的时间
	Timestamp m_calcNetLastTime;
	// kcp底层对象
	ikcpcb* m_kcp;
	// kcp update timer
	TimerId m_kcpUpdateTimerId;
	// 是否是client
	const bool m_isClient;
	// 密钥
	const int m_secretId;
	// kcp 配置
	KcpSettings m_kcpSet;
	// 事件
	std::unique_ptr<Channel> m_udpChannel;
	// 握手超时
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
