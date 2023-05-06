// Comment: 一些回调类型声明

#ifndef _SZNET_BASE_CALLBACKS_H_
#define _SZNET_BASE_CALLBACKS_H_

#include "../base/Types.h"
#include "../time/Timestamp.h"

#include <functional>
#include <memory>

namespace sznet
{

// 获取智能指针中的地址
template<typename T>
inline T* get_pointer(const std::shared_ptr<T>& ptr)
{
	return ptr.get();
}
template<typename T>
inline T* get_pointer(const std::unique_ptr<T>& ptr)
{
	return ptr.get();
}

// 智能指针安全的向下转换
template<typename To, typename From>
inline ::std::shared_ptr<To> down_pointer_cast(const ::std::shared_ptr<From>& f) {
	if (false)
	{
		implicit_cast<From*, To*>(0);
	}

#ifndef NDEBUG
	assert(f == NULL || dynamic_cast<To*>(get_pointer(f)) != NULL);
#endif
	// static_pointer_cast
	// Creates a new instance of std::shared_ptr whose stored pointer is obtained from r's stored pointer using a cast expression.
	// If r is empty, so is the new shared_ptr(but its stored pointer is not necessarily null).
	// Otherwise, the new shared_ptr will share ownership with the initial value of r, 
	// except that it is empty if the dynamic_cast performed by dynamic_pointer_cast returns a null pointer.
	return ::std::static_pointer_cast<To>(f);
}

namespace net
{

// 网络消息缓冲区前置声明
class Buffer;
// TCP连接前置声明
class TcpConnection;
// TCP连接的智能指针类型
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
// Timer到期回调函数类型
typedef std::function<void()> TimerCallback;
// TcpConnection相关回调函数类型
// 外部连接建立/断开回调函数
typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;
// 内部连接断开回调函数
typedef std::function<void(const TcpConnectionPtr&)> CloseCallback;
// 数据发送完成的回调函数，直接发送成功/发送缓冲区发送完毕
typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
// 发送高水位线回调函数，上升沿触发一次
typedef std::function<void(const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;
// 外部处理收到消息的回调调函数
typedef std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)> MessageCallback;
// 默认外部连接建立/断开回调函数
void defaultConnectionCallback(const TcpConnectionPtr& conn);
// 默认外部处理收到消息的回调调函数
void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime);

} // end namespace net

} // end namespace sznet

#endif // _SZNET_BASE_CALLBACKS_H_
