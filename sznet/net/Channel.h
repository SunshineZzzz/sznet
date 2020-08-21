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
	// �¼��ص�����
	typedef std::function<void()> EventCallback;
	// ���¼��ص�����
	typedef std::function<void(Timestamp)> ReadEventCallback;

	Channel(EventLoop* loop, int fd);
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
	int fd() const 
	{ 
		return m_fd; 
	}
	// ����fdע����¼�
	int events() const 
	{ 
		return m_events; 
	}
	// ���ж�·����API�󣬸���fd�ķ����¼����ô˺���, �趨fd�ľ����¼�����
	// handleEvent���ݾ����¼�����(m_revents)������ִ���ĸ��¼��ص�����
	void set_revents(int revt) 
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
	// 
	void remove();

private:
	// 
	static string eventsToString(int fd, int ev);
	// updateͨ������m_loop->updateChannel()��ע���ı��fd�ڶ�·�����е�ע����¼�
	void update();
	// 
	void handleEventWithGuard(Timestamp receiveTime);

	// ���¼�
	static const int kNoneEvent;
	// �ɶ��¼�
	static const int kReadEvent;
	// ��д�¼�
	static const int kWriteEvent;

	// 
	EventLoop* m_loop;
	// ��Channel������ļ���������Channel��ӵ��fd
	const int m_fd;
	// fdע����¼�
	int m_events;
	// ͨ����·����API��ľ����¼�
	int m_revents;
	// 
	int m_index;
	// �Ƿ�����ĳЩ��־
	bool m_logHup;
	// 
	std::weak_ptr<void> m_tie;
	// 
	bool m_tied;
	// 
	bool m_eventHandling;
	// 
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
