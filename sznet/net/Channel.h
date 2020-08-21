#ifndef _SZNET_NET_CHANNEL_H_
#define _SZNET_NET_CHANNEL_H_

#include "../base/NonCopyable.h"
#include "../time/Timestamp.h"

#include <functional>
#include <memory>

namespace sznet
{

namespace net
{

// 
class EventLoop;

// 
class Channel : NonCopyable
{
public:
	// 事件回调函数
	typedef std::function<void()> EventCallback;
	// 读事件回调函数
	typedef std::function<void(Timestamp)> ReadEventCallback;

	Channel(EventLoop* loop, int fd);
	~Channel();

	// 
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
	// 
	void tie(const std::shared_ptr<void>&);
	// 返回该Channel负责的fd
	int fd() const 
	{ 
		return m_fd; 
	}
	// 返回fd注册的事件
	int events() const 
	{ 
		return m_events; 
	}
	// 进行多路复用API后，根据fd的返回事件调用此函数, 设定fd的就绪事件类型
	// handleEvent根据就绪事件类型(m_revents)来决定执行哪个事件回调函数
	void set_revents(int revt) 
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
	// 
	int index() 
	{ 
		return m_index; 
	}
	// 
	void set_index(int idx) 
	{ 
		m_index = idx;
	}
	// 
	string reventsToString() const;
	// 
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
	// 
	void remove();

private:
	// 
	static string eventsToString(int fd, int ev);
	// update通过调用m_loop->updateChannel()来注册或改变该fd在多路复用中的注册的事件
	void update();
	// 
	void handleEventWithGuard(Timestamp receiveTime);

	// 无事件
	static const int kNoneEvent;
	// 可读事件
	static const int kReadEvent;
	// 可写事件
	static const int kWriteEvent;

	// 
	EventLoop* m_loop;
	// 本Channel负责的文件描述符，Channel不拥有fd
	const int m_fd;
	// fd注册的事件
	int m_events;
	// 通过多路复用API后的就绪事件
	int m_revents;
	// 
	int m_index;
	// 是否生成某些日志
	bool m_logHup;
	// 
	std::weak_ptr<void> m_tie;
	// 
	bool m_tied;
	// 
	bool m_eventHandling;
	// 
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
