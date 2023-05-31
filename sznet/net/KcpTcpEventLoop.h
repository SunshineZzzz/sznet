// Comment: KCP&TCP事件循环实现

#ifndef _SZNET_NET_KCPTCPEVENTLOOP_H_
#define _SZNET_NET_KCPTCPEVENTLOOP_H_

#include "EventLoop.h"
#include "Socket.h"
#include "InetAddress.h"
#include "Channel.h"
#include "Buffer.h"

#include <random>
#include <map>
#include <unordered_map>

namespace sznet
{

namespace net
{

// KCP&TCP IO事件循环
class KcpTcpEventLoop : public EventLoop
{
public:
	struct sockaddrInKeyHash {
		std::size_t operator()(const struct sockaddr_in& k) const
		{
			return k.sin_addr.s_addr + k.sin_port;
		}
	};
	struct sockaddrInKeyEqual {
		bool operator()(const struct sockaddr_in& lhs, const struct sockaddr_in& rhs) const
		{
			return memcmp(&lhs, &rhs, sizeof(sockaddr_in)) == 0;
		}
	};
	// TCP连接管理器类型
	typedef std::map<uint32_t, TcpConnectionPtr> TcpConnectionMap;
	// KCP连接管理器类型
	typedef std::map<uint32_t, KcpConnectionPtr> KcpConnectionMap;
	// 对端地址管理器类型
	typedef std::unordered_map<sockaddr_in, KcpConnectionPtr, sockaddrInKeyHash, sockaddrInKeyEqual> AddrConnHashtable;
	// 密钥管理器
	typedef std::map<uint32_t, int32_t> cryptConvMap;

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

public:
	KcpTcpEventLoop(const InetAddress udpLisetenAddr, string name = string(), int seed = 0);
	virtual ~KcpTcpEventLoop();

	// 返回线程监听的UDP端口
	const uint16_t getUdpListenPort() const
	{
		return m_udpLisetenAddr.port();
	}
	// 返回线程监听的UDP端口和地址
	string getUdpListenIpPort()
	{
		return m_udpLisetenAddr.toIpPort();
	}
	// 返回临时缓冲区
	Buffer& getTmpBuffer()
	{
		return m_tmpBuffer;
	}
	// tcp管理器
	TcpConnectionMap& getTcpConnections()
	{
		return m_tcpConnections;
	}
	// kcp管理器
	KcpConnectionMap& getKcpConnections()
	{
		return m_kcpConnections;
	}
	// 密钥管理器
	cryptConvMap& getCrypts()
	{
		return m_crypts;
	}
	// 生成密钥
	int genSecret()
	{
		return m_random();
	}
	// 获取名称
	string& getName()
	{
		return m_name;
	}
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
	// TCP连接断开了
	void removeTcpConnectionInLoop(const TcpConnectionPtr& conn);
	// KCP连接断开了
	void removeKcpConnectionInLoop(const KcpConnectionPtr& conn, const int oldState);

private:
	// UDP可读事件
	void handleUdpRead(Timestamp receiveTime);
	// 新来的UDP消息
	void handleNewUdpMessage();
	// KCP消息
	void handleKcpMessage();
	// UDP send
	void checkResponse(bool isSuccess);
	// 各种容器断言
	void assertVessels()
	{
		assert(m_tcpConnections.size() == m_kcpConnections.size());
		assert(m_tcpConnections.size() == m_addrConns.size());
		assert(m_tcpConnections.size() == m_crypts.size());
		assert(m_tcpConnections.size() == m_addrConns.size());
	}

	// udp监听地址
	InetAddress m_udpLisetenAddr;
	// udp监听Socket
	Socket m_udpSocket;
	// udp事件
	Channel m_udpChannel;
	// UDP接收缓冲区
	Buffer m_inputBuffer;
	// 临时缓冲
	Buffer m_tmpBuffer;
	// id <-> tcp conn
	TcpConnectionMap m_tcpConnections;
	// id <-> kcp conn
	KcpConnectionMap m_kcpConnections;
	// sockaddr_in <-> kcp conn
	// UDP完成握手以后才会有呢
	AddrConnHashtable m_addrConns;
	// 当前网络数据
	NetStatInfo m_netStatInfo;
	// 上一次网络数据
	NetStatInfo m_prevNetStatInfo;
	// 上一次统计网络的时间
	Timestamp m_calcNetLastTime;
	// 密钥生成器
	std::mt19937 m_random;
	// 连接ID <-> 密钥
	cryptConvMap m_crypts;
	// 当前正在处理的收到消息的addr
	sockaddr_in m_curRecvAddr;
	// 当前正在处理的连接ID
	uint32_t m_curConv;
	// 当前正在操作的tcp conn
	TcpConnectionPtr m_curTcpConn;
	// 当前正在操作的kcp conn
	KcpConnectionPtr m_curKcpConn;
	// 名称
	string m_name;
	// Kcp连接/断开通知回调函数
	KcpConnectionCallback m_kcpConnectionCallback;
	// KCP处理收到消息的回调调函数
	KcpMessageCallback m_kcpMessageCallback;
	// TCP数据发送完成的回调函数，直接发送成功/发送缓冲区发送完毕
	KcpWriteCompleteCallback m_kcpWriteCompleteCallback;
};

} // end namespace net

} // end namespace sznet

#endif  // _SZNET_NET_KCPTCPEVENTLOOP_H_
