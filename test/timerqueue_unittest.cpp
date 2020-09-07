﻿#include "../sznet/net/EventLoop.h"
#include "../sznet/net/EventLoopThread.h"
#include "../sznet/log/Logging.h"
#include "../sznet/thread/ThreadClass.h"
#include "../sznet/thread/CurrentThread.h"

#include <string>
#include <functional>
#include <iostream>

#if SZ_COMPILER_MSVC
#	pragma comment(lib, "sznet.lib")
#endif

using namespace sznet;
using namespace sznet::net;

int cnt = 0;
EventLoop* g_timequeue_loop = nullptr;

void printid()
{
	LOG_INFO << "pid = " << CurrentThread::tid();
	LOG_INFO << "now " << Timestamp::now().toFormattedString();
}

void print(const char* msg)
{
	LOG_INFO << "time:" << Timestamp::now().toFormattedString() << " msg:" << msg;
	if (++cnt == 20)
	{
		g_timequeue_loop->quit();
	}
}

void cancel(TimerId timer)
{
	g_timequeue_loop->cancel(timer);
	LOG_INFO << "cancelled at " << Timestamp::now().toFormattedString();
}

int main()
{
	printid();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	{
		EventLoop loop;
		g_timequeue_loop = &loop;

		LOG_INFO << "main";
		loop.runAfter(1, std::bind(print, "once1"));
		loop.runAfter(1.5, std::bind(print, "once1.5"));
		loop.runEvery(2, std::bind(print, "every2"));
		loop.runAfter(2.5, std::bind(print, "once2.5"));
		loop.runAfter(3.5, std::bind(print, "once3.5"));

		TimerId t45 = loop.runAfter(4.5, std::bind(print, "once4.5"));
		loop.runAfter(4.2, std::bind(cancel, t45));
		loop.runAfter(4.8, std::bind(cancel, t45));
		TimerId t3 = loop.runEvery(3, std::bind(print, "every3"));
		loop.runAfter(9.001, std::bind(cancel, t3));

		loop.loop();
		LOG_INFO << "main loop exits";
	}
	std::this_thread::sleep_for(std::chrono::seconds(1));
	{
		EventLoopThread loopThread;
		EventLoop* loop = loopThread.startLoop();
		loop->runAfter(2, printid);
		std::this_thread::sleep_for(std::chrono::seconds(3));
		LOG_INFO << "thread loop exits";
	}
	std::cin.get();
	return 0;
}