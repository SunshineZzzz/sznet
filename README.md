# SZNET

## 介绍

SZNET核心代码都是来自[muduo](https://github.com/chenshuo/muduo)，将其进行了扩展，并且支持跨平台，如果你对跨平台编程感兴趣，对网络编程感兴趣，对multiple reactors感兴趣，对KCP如何接入已有的multiple reactors感兴趣都可以看看。

test目录下有，KCP TCP UDP 基于multiple reactors的echo服务器与客户端实现

## 编译

```
windows：
sznet/sln/sznet.sln，使用的是vs2019
```

```
linux
$ mkdir cmake_build
$ cd cmake_build
$ cmake ..
$ make all
```

## 目录结构
```
.
├── 3rd 第三方库相关
│   └── kcp KCP相关
├── CMakeLists.txt
├── README.md
├── sln vs工程
├── sznet 核心库
│   ├── base
│   │   ├── Callbacks.h 一些回调类型声明
│   │   ├── Compiler.h 编译环境宏定义
│   │   ├── Copyable.h 可拷贝基类的实现
│   │   ├── Exception.h 异常基类实现
│   │   ├── NonCopyable.h 不可拷贝基类实现
│   │   ├── Platform.h 所处平台宏定义
│   │   ├── Types.h 安全转换
│   │   └── WeakCallback.h 弱回调实现
│   ├── ds
│   │   └── MinHeap.h 最小堆实现
│   ├── io
│   │   ├── FileUtil.cpp 
│   │   ├── FileUtil.h 文件操作
│   │   ├── IO.cpp
│   │   └── IO.h IO相关接口
│   ├── log
│   │   ├── AsyncLogging.cpp
│   │   ├── AsyncLogging.h 配合LogFile实现异步日志文件
│   │   ├── LogFile.cpp
│   │   ├── LogFile.h 日志文件实现
│   │   ├── Logging.cpp
│   │   ├── Logging.h 日志实现
│   │   ├── LogStream.cpp
│   │   └── LogStream.h 日志流缓冲区实现
│   ├── net
│   │   ├── Acceptor.cpp
│   │   ├── Acceptor.h 连接接收器实现
│   │   ├── Buffer.cpp
│   │   ├── Buffer.h 网络消息缓冲区实现
│   │   ├── Channel.cpp
│   │   ├── Channel.h 事件分发
│   │   ├── Codec.h 4字节编解码器实现
│   │   ├── Connector.cpp
│   │   ├── Connector.h 主动连接实现
│   │   ├── DefaultPoller.cpp
│   │   ├── Endian.h 网络字节序相关函数接口
│   │   ├── EventLoop.cpp
│   │   ├── EventLoop.h 事件循环实现
│   │   ├── EventLoopThread.cpp
│   │   ├── EventLoopThread.h 线程IO事件循环实现
│   │   ├── EventLoopThreadPool.cpp
│   │   ├── EventLoopThreadPool.h 线程池事件IO循环实现
│   │   ├── InetAddress.cpp
│   │   ├── InetAddress.h 网络地址实现
│   │   ├── KcpConnection.cpp
│   │   ├── KcpConnection.h KCP连接器
│   │   ├── KcpTcpEventLoop.cpp
│   │   ├── KcpTcpEventLoop.h KCP&TCP事件循环实现
│   │   ├── KcpTcpEventLoopThread.cpp
│   │   ├── KcpTcpEventLoopThread.h KCP&TCP线程IO事件循环实现
│   │   ├── KcpTcpEventLoopThreadPool.cpp
│   │   ├── KcpTcpEventLoopThreadPool.h KCP&TCP线程池事件IO循环实现
│   │   ├── KcpWithTcpClient.cpp
│   │   ├── KcpWithTcpClient.h KcpWithTcp客户端实现
│   │   ├── KcpWithTcpServer.cpp
│   │   ├── KcpWithTcpServer.h  KcpWithTcp服务器实现
│   │   ├── Poller.cpp
│   │   ├── Poller.h IO复用基类实现
│   │   ├── SelectPoller.cpp
│   │   ├── SelectPoller.h select IO复用实现
│   │   ├── Socket.cpp
│   │   ├── Socket.h socket实现
│   │   ├── SocketsOps.cpp
│   │   ├── SocketsOps.h 网络相关接口
│   │   ├── TcpClient.cpp
│   │   ├── TcpClient.h TCP客户端实现
│   │   ├── TcpConnection.cpp
│   │   ├── TcpConnection.h TCP连接实现
│   │   ├── TcpServer.cpp
│   │   ├── TcpServer.h TCP服务器实现
│   │   ├── UdpOps.cpp
│   │   └── UdpOps.h UDP相关接口
│   ├── NetCmn.cpp
│   ├── NetCmn.h 公共头文件
│   ├── process
│   │   ├── Process.cpp
│   │   └── Process.h 进程相关函数接口
│   ├── string
│   │   ├── StringOpt.cpp
│   │   ├── StringOpt.h 字符串相关操作
│   │   └── StringPiece.h string view实现
│   ├── thread
│   │   ├── Condition.h 条件变量实现
│   │   ├── CountDownLatch.cpp
│   │   ├── CountDownLatch.h 倒计时计数器实现
│   │   ├── CurrentThread.cpp
│   │   ├── CurrentThread.h  当前线程信息
│   │   ├── Mutex.h 互斥量实现
│   │   ├── ThreadClass.cpp
│   │   ├── ThreadClass.h 线程实现
│   │   ├── Thread.cpp
│   │   ├── Thread.h 线程和线程同步接口
│   │   ├── ThreadPool.cpp
│   │   └── ThreadPool.h 线程池实现
│   └── time
│       ├── Time.cpp
│       ├── Time.h 时间相关接口
│       ├── Timer.cpp
│       ├── Timer.h 定时器实现
│       ├── TimerId.h 暴露给使用者的定时器对象，方便多次取消定时器
│       ├── TimerQueue.cpp
│       ├── TimerQueue.h 定时器队列的实现
│       ├── Timestamp.cpp
│       └── Timestamp.h 时间戳实现
└── test 各种测试代码
```