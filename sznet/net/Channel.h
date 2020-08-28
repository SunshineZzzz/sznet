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

// �¼�ѭ��
class EventLoop;

// �¼��ַ�
class Channel : NonCopyable
{
public:
	// �¼�����
	enum 
	{
		// ��
		kNoneEvent = 0x00,
		// �Ƿ�ɶ�
		kReadEvent = 0x01,
		// �Ƿ��д
		kWriteEvent = 0x02,
		// �Ƿ�ر�
		kCloseEvent = 0x04,
		// �Ƿ����
		kErrorEvent = 0x08,
	};
	// �¼�
	struct Event_t
	{
		sockets::sz_fd fd;
		unsigned char ev;
		Event_t()
		{
			clear();
		}
		inline void clear()
		{
			fd = sz_invalid_socket;
			ev = kNoneEvent;
		}
	};

public:
	// �¼��ص�����
	typedef std::function<void()> EventCallback;
	// ���¼��ص�����
	typedef std::function<void(Timestamp)> ReadEventCallback;

	Channel(EventLoop* loop, sockets::sz_sock fd);
	~Channel();

	// 
	void handleEvent(Timestamp receiveTime);
	// ���ÿɶ��ص�����
	void setReadCallback(ReadEventCallback cb)
	{
		m_readCallback = std::move(cb);
	}
	// ����д�ص�����
	void setWriteCallback(EventCallback cb)
	{
		m_writeCallback = std::move(cb);
	}
	// ���ùرջص�����
	void setCloseCallback(EventCallback cb)
	{
		m_closeCallback = std::move(cb);
	}
	// ���ô���ص�����
	void setErrorCallback(EventCallback cb)
	{
		m_errorCallback = std::move(cb);
	}
	// 
	void tie(const std::shared_ptr<void>&);
	// ���ظ�Channel�����fd
	sockets::sz_fd fd() const 
	{ 
		return m_fd; 
	}
	// ����fdע����¼�
	unsigned char events() const
	{ 
		return m_events; 
	}
	// ���ж�·����API�󣬸���fd�ķ����¼����ô˺���, �趨fd�ľ����¼�����
	// handleEvent���ݾ����¼�����(m_revents)������ִ���ĸ��¼��ص�����
	void set_revents(unsigned char revt)
	{ 
		m_revents = revt;
	}
	// ����m_revents
	// int revents() const 
	// { 
	//	 return m_revents;
	// }
	// �ж�fd�ǲ���û���¼�����
	bool isNoneEvent() const 
	{ 
		return m_events == kNoneEvent;
	}
	// fdע��ɶ��¼�
	void enableReading() 
	{ 
		m_events |= kReadEvent; 
		update();
	}
	// ����fd�ɶ��¼�
	void disableReading() 
	{ 
		m_events &= ~kReadEvent; 
		update();
	}
	// fdע���д�¼�
	void enableWriting() 
	{ 
		m_events |= kWriteEvent; 
		update();
	}
	// ����fd��д�¼�
	void disableWriting() 
	{ 
		m_events &= ~kWriteEvent; 
		update();
	}
	// ֹͣfd�����¼�
	void disableAll() 
	{ 
		m_events = kNoneEvent; 
		update();
	}
	// �Ƿ�ע����д�¼�
	bool isWriting() const 
	{ 
		return m_events & kWriteEvent;
	}
	// �Ƿ�ע���˶��¼�
	bool isReading() const 
	{ 
		return m_events & kReadEvent;
	}
	// ��ȡ�±�
	int index() 
	{ 
		return m_index; 
	}
	// �����±�
	void set_index(int idx) 
	{ 
		m_index = idx;
	}
	// 
	string reventsToString() const;
	// 
	string eventsToString() const;
	// �ر���־��ӡ
	void doNotLogHup() 
	{ 
		m_logHup = false; 
	}
	// ���س��б�Channel��EventLoopָ�� 
	EventLoop* ownerLoop() 
	{ 
		return m_loop; 
	}
	// ��Channel��EventLoop���Ƴ�
	void remove();

private:
	// 
	static string eventsToString(sockets::sz_sock fd, int ev);
	// updateͨ������m_loop->updateChannel()��ע���ı��fd�ڶ�·�����е�ע����¼�
	void update();
	// 
	void handleEventWithGuard(Timestamp receiveTime);

	// �¼�ѭ��
	EventLoop* m_loop;
	// ��Channel������ļ���������Channel��ӵ��fd
	const sockets::sz_fd m_fd;
	// fdע����¼�
	unsigned char m_events;
	// ͨ����·����API��ľ����¼�
	unsigned char m_revents;
	// poller�б���channel���������±�
	// ͨ�����Ҳ�����ж���add�¼�����mod�¼�
	int m_index;
	// �Ƿ�����ĳЩ��־
	bool m_logHup;
	// 
	std::weak_ptr<void> m_tie;
	// 
	bool m_tied;
	// 
	bool m_eventHandling;
	// �¼��Ƿ��Ѿ�ע�ᵽloop��
	bool m_addedToLoop;
	// ���¼��ص�����
	ReadEventCallback m_readCallback;
	// д�¼��ص�����
	EventCallback m_writeCallback;
	// �ر��¼��ص�����
	EventCallback m_closeCallback;
	// �����¼��ص�����
	EventCallback m_errorCallback;
};

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_CHANNEL_H_
