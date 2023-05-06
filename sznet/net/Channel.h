// Comment: 事件分发

#ifndef _SZNET_NET_CHANNEL_H_
#define _SZNET_NET_CHANNEL_H_

#include "SocketsOps.h"
#include "../base/NonCopyable.h"
#include "../time/Timestamp.h"

#include <functional>
#include <memory>

namespace sznet
{

namespace net
{

// 事件循环
class EventLoop;

// 事件分发
class Channel : NonCopyable
{
public:
	// 事件类型
	// https://stackoverflow.com/questions/52976152/tcp-when-is-epollhup-generated
	// https://stackoverflow.com/questions/56177060/pollhup-vs-pollrdhup
	// https://stackoverflow.com/questions/24791625/how-to-handle-the-linux-socket-revents-pollerr-pollhup-and-pollnval
	// You can have a look at net/ipv4/tcp.c the kernel source:
	// if (sk->sk_shutdown == SHUTDOWN_MASK || state == TCP_CLOSE)
	//		mask |= EPOLLHUP;
	// if (sk->sk_shutdown & RCV_SHUTDOWN)
	//		mask |= EPOLLIN | EPOLLRDNORM | EPOLLRDHUP;
	// SHUTDOWN_MASK is RCV_SHUTDOWN | SEND_SHUTDOWN. 
	// RCV_SHUTDOWN is set when a FIN packet is received, 
	// and SEND_SHUTDOWN is set when a FIN packet is acknowledged by the other end, 
	// and the socket moves to the FIN - WAIT2 state.
	enum 
	{
		// none
		kNoneEvent = 0x00,
		// POLLIN，EPOLLIN
		// There is data to read
		kReadEvent = 0x01,
		// POLLPRI，EPOLLPRI
		// out-of-band data on TCP socket
		kPriEvent = 0x02,
		// POLLRDHUP，EPOLLRDHUP since Linux 2.6.17
		// POLLRDHUP will be set when the other end has called shutdown(SHUT_WR) or 
		// when this end has called shutdown(SHUT_RD), 
		// but the connection may still be alive in the other direction.
		kRdHupEvent = 0x04,
		// POLLOUT，EPOLLOUT
		// The associated file is available for write operations.
		kWriteEvent = 0x08,
		// POLLERR，EPOLLERR
		// Error condition happened on the associated file descriptor.
		kErrorEvent = 0x10,
		// POLLHUP，EPOLLHUP
		// when poll()ing a socket, POLLHUP will signal that the connection was closed in both directions.
		kHupEvent = 0x20,
		// POLLNVAL
		// A POLLNVAL means the socket file descriptor is not open. 
		// It would be an error to close() it.
		kNvalEvent = 0x40,
		// kCloseEvent = 0x80,
	};
	// 事件
	struct Event_t
	{
		sockets::sz_fd fd;
		unsigned int ev;
		Event_t()
		{
			clear();
		}
		void clear()
		{
			fd = sz_invalid_socket;
			ev = kNoneEvent;
		}
	};

public:
	// 事件回调函数
	typedef std::function<void()> EventCallback;
	// 读事件回调函数
	typedef std::function<void(Timestamp)> ReadEventCallback;

	Channel(EventLoop* loop, sockets::sz_sock fd);
	~Channel();

	// 处理事件
	void handleEvent(Timestamp receiveTime);
	// 设置可读回调函数
	void setReadCallback(ReadEventCallback cb)
	{
		m_readCallback = std::move(cb);
	}
	// 设置写回调函数
	void setWriteCallback(EventCallback cb)
	{
		m_writeCallback = std::move(cb);
	}
	// 设置关闭回调函数
	void setCloseCallback(EventCallback cb)
	{
		m_closeCallback = std::move(cb);
	}
	// 设置错误回调函数
	void setErrorCallback(EventCallback cb)
	{
		m_errorCallback = std::move(cb);
	}
	// 设置m_tie
	void tie(const std::shared_ptr<void>&);
	// 返回该Channel负责的fd
	sockets::sz_fd fd() const 
	{ 
		return m_fd; 
	}
	// 返回fd注册的事件
	unsigned int events() const
	{ 
		return m_events; 
	}
	// 进行多路复用API后，根据fd的返回事件调用此函数, 设定fd的就绪事件类型
	// handleEvent根据就绪事件类型(m_revents)来决定执行哪个事件回调函数
	void set_revents(unsigned int revt)
	{ 
		m_revents = revt;
	}
	// 返回m_revents
	// int revents() const 
	// { 
	//	 return m_revents;
	// }
	// 判断fd是不是没有事件监听
	bool isNoneEvent() const 
	{ 
		return m_events == kNoneEvent;
	}
	// fd注册可读事件
	void enableReading() 
	{ 
		m_events |= kReadEvent; 
		update();
	}
	// 销毁fd可读事件
	void disableReading() 
	{ 
		m_events &= ~kReadEvent; 
		update();
	}
	// fd注册可写事件
	void enableWriting() 
	{ 
		m_events |= kWriteEvent; 
		update();
	}
	// 销毁fd可写事件
	void disableWriting() 
	{ 
		m_events &= ~kWriteEvent; 
		update();
	}
	// 停止fd所有事件
	void disableAll() 
	{ 
		m_events = kNoneEvent; 
		update();
	}
	// 是否注册了写事件
	bool isWriting() const 
	{ 
		return m_events & kWriteEvent;
	}
	// 是否注册了读事件
	bool isReading() const 
	{ 
		return m_events & kReadEvent;
	}
	// 获取下标
	int index() 
	{ 
		return m_index; 
	}
	// 设置下标
	void set_index(int idx) 
	{ 
		m_index = idx;
	}
	// pool后的事件到字符串
	string reventsToString() const;
	// 注册事件到字符串
	string eventsToString() const;
	// 关闭日志打印
	void doNotLogHup() 
	{ 
		m_logHup = false; 
	}
	// 返回持有本Channel的EventLoop指针 
	EventLoop* ownerLoop() 
	{ 
		return m_loop; 
	}
	// 将Channel从EventLoop中移除
	void remove();

private:
	// 打印ev对应的字符串
	static string eventsToString(sockets::sz_sock fd, int ev);
	// update通过调用m_loop->updateChannel()来注册或改变该fd在多路复用中的注册的事件
	void update();
	// 保护的处理事件
	void handleEventWithGuard(Timestamp receiveTime);

	// 事件循环
	EventLoop* m_loop;
	// 本Channel负责的文件描述符，Channel不拥有fd
	const sockets::sz_fd m_fd;
	// fd注册的事件
	unsigned int m_events;
	// 通过多路复用API后的就绪事件
	unsigned int m_revents;
	// 数组下标，poller中保存channel的数组的下标
	// 通过这个也可以判断是add事件还是mod事件
	int m_index;
	// 是否输出POLLHUP挂起日志
	bool m_logHup;
	// 防止owner对象析构，延长owner生命期
	std::weak_ptr<void> m_tie;
	// 是否已经设置m_tie
	bool m_tied;
	// 当前是否正在处理事件
	bool m_eventHandling;
	// 事件是否已经注册到loop中
	bool m_addedToLoop;
	// 读事件回调函数
	ReadEventCallback m_readCallback;
	// 写事件回调函数
	EventCallback m_writeCallback;
	// 关闭事件回调函数
	EventCallback m_closeCallback;
	// 错误事件回调函数
	EventCallback m_errorCallback;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_CHANNEL_H_
