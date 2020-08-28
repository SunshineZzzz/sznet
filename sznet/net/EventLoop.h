#ifndef _SZNET_NET_EVENTLOOP_H_
#define _SZNET_NET_EVENTLOOP_H_

#include "../thread/Mutex.h"
#include "../thread/CurrentThread.h"
#include "../time/Timestamp.h"
#include "../base/NonCopyable.h"
#include "../base/Callbacks.h"
#include "../time/TimerId.h"
#include "../net/SocketsOps.h"

#include <atomic>
#include <functional>
#include <vector>

namespace sznet
{

// ��ʱ������
class TimerQueue;

namespace net
{

// �¼��ַ�
class Channel;
// ��·����
class Poller;

// �¼�ѭ��
class EventLoop : NonCopyable
{
public:
	// 
	typedef std::function<void()> Functor;

	EventLoop();
	~EventLoop();

	// �¼�ѭ��
	void loop();
	// ��ֹ�¼�ѭ��
	void quit();
	// 
	// Timestamp pollReturnTime() const { return pollReturnTime_; }
	// 
	// int64_t iteration() const { return iteration_; }
	// 
	void runInLoop(Functor cb);
	// 
	void queueInLoop(Functor cb);
	// 
	// size_t queueSize() const;
	// timers
	// 
	TimerId runAt(Timestamp time, TimerCallback cb);
	// 
	TimerId runAfter(double delay, TimerCallback cb);
	// 
	TimerId runEvery(double interval, TimerCallback cb);
	// ȡ����ʱ��
	void cancel(TimerId timerId);
	// ����EventLoop���ڵ��̣߳�����ֹͣ�¼�ѭ��
	void wakeup();
	// ����channel���¼�
	void updateChannel(Channel* channel);
	// �Ƴ�channel
	void removeChannel(Channel* channel);
	// bool hasChannel(Channel* channel);
	// pid_t threadId() const { return threadId_; }
	// ��������loop�������̱߳�����ӵ��EventLoop������߳�
	void assertInLoopThread()
	{
		// �������̲߳�ӵ��EventLoop�������˳�����֤one loop per thread
		if (!isInLoopThread())
		{
			abortNotInLoopThread();
		}
	}
	// �жϵ�ǰ�߳��Ƿ���ӵ�д�EventLoop���߳�
	bool isInLoopThread() const 
	{ 
		return m_threadId == CurrentThread::tid();
	}
	// bool callingPendingFunctors() const { return callingPendingFunctors_; }
	// bool eventHandling() const { return eventHandling_; }
	// void setContext(const boost::any& context)
	// {
	//	context_ = context;
	// }
	// const boost::any& getContext() const
	// {
	// 	return context_;
	// }
	// 
	// boost::any* getMutableContext()
	// {
	// 	return &context_;
	// }
	// 
	// static EventLoop* getEventLoopOfCurrentThread();

private:
	// �ڲ�ӵ��EventLoop�߳�����ֹ
	void abortNotInLoopThread();
	// m_wakeupFd�ɶ�ʱ�ص�����
	void handleRead();
	// 
	void doPendingFunctors();
	// 
	void printActiveChannels() const;
	// 
	typedef std::vector<Channel*> ChannelList;
	// �¼�ѭ���Ƿ�����
	bool m_looping;
	// �¼�ѭ��ǰ���жϸñ������˳��¼�ѭ�����
	std::atomic<bool> m_quit;
	// �Ƿ����ڴ����¼�
	bool m_eventHandling;
	// �Ƿ�����ִ���û��������ص�
	bool m_callingPendingFunctors;
	// loop����
	int64_t m_iteration;
	// ��ǰ�̵߳�tid
	const sz_pid_t m_threadId;
	// 
	Timestamp pollReturnTime_;
	// ��·���ö���
	std::unique_ptr<Poller> m_poller;
	// ��ʱ���������ڴ�Ŷ�ʱ��
	std::unique_ptr<TimerQueue> m_timerQueue;
	// ����EventLoop���ڵ��߳�
	sockets::sz_event m_wakeupFd;
	// m_wakeupFd��Ӧ���¼��ַ�����
	std::unique_ptr<Channel> m_wakeupChannel;
	// 
	// boost::any context_;
	// 
	ChannelList m_activeChannels;
	// ��ǰ���ڴ���ľ����¼�
	Channel* m_currentActiveChannel;
	// 
	mutable MutexLock mutex_;
	// 
	std::vector<Functor> pendingFunctors_;
};

} // end namespace net

} // end namespace muduo

#endif  // MUDUO_NET_EVENTLOOP_H
