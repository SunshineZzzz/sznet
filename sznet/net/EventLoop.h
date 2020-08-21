#ifndef _SZNET_NET_EVENTLOOP_H_
#define _SZNET_NET_EVENTLOOP_H_

#include "../thread/Mutex.h"
#include "../thread/CurrentThread.h"
#include "../time/Timestamp.h"
#include "../base/NonCopyable.h"
#include "../base/Callbacks.h"
#include "../time/TimerId.h"

#include <atomic>
#include <functional>
#include <vector>

namespace sznet
{

namespace net
{

// 
class Channel;
// 
class Poller;
// 
class TimerQueue;

// 
class EventLoop : NonCopyable
{
public:
	// 
	typedef std::function<void()> Functor;

	EventLoop();
	~EventLoop();

	// 
	// void loop();
	// ��ֹ�¼�ѭ��
	void quit();
	// 
	// Timestamp pollReturnTime() const { return pollReturnTime_; }
	// 
	// int64_t iteration() const { return iteration_; }
	// 
	// void runInLoop(Functor cb);
	// 
	// void queueInLoop(Functor cb);
	// 
	// size_t queueSize() const;
	// timers
	// 
	TimerId runAt(Timestamp time, TimerCallback cb);
	// 
	TimerId runAfter(double delay, TimerCallback cb);
	// 
	TimerId runEvery(double interval, TimerCallback cb);
	// 
	// void cancel(TimerId timerId);
	// ����EventLoop���ڵ��̣߳�����ֹͣ�¼�ѭ��
	void wakeup();
	// void updateChannel(Channel* channel);
	// void removeChannel(Channel* channel);
	// bool hasChannel(Channel* channel);
	// pid_t threadId() const { return threadId_; }
	// ����loop�������̱߳�����ӵ��EventLoop���߳�
	void assertInLoopThread()
	{
		if (!isInLoopThread())
		{
			abortNotInLoopThread();
		}
	}
	// 
	// 
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
	// 
	void handleRead();
	// 
	void doPendingFunctors();
	// 
	void printActiveChannels() const;
	// 
	typedef std::vector<Channel*> ChannelList;
	// �¼�ѭ���Ƿ����б��
	bool m_looping;
	// �¼�ѭ��ǰ���жϸñ��
	std::atomic<bool> m_quit;
	// 
	bool eventHandling_;
	// 
	bool callingPendingFunctors_;
	// 
	int64_t iteration_;
	// ��ǰ�̵߳�tid
	const sz_pid_t m_threadId;
	// 
	Timestamp pollReturnTime_;
	// 
	std::unique_ptr<Poller> poller_;
	// 
	std::unique_ptr<TimerQueue> timerQueue_;
	// 
	// int wakeupFd_;
	// 
	// std::unique_ptr<Channel> wakeupChannel_;
	// 
	// boost::any context_;
	// 
	ChannelList activeChannels_;
	// 
	Channel* currentActiveChannel_;
	// 
	mutable MutexLock mutex_;
	// 
	std::vector<Functor> pendingFunctors_;
};

} // end namespace net

} // end namespace muduo

#endif  // MUDUO_NET_EVENTLOOP_H
