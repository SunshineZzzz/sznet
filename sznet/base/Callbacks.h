#ifndef _SZNET_NET_CALLBACKS_H_
#define _SZNET_NET_CALLBACKS_H_

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

// 
// class Buffer;
// 
// class TcpConnection;
// 
// typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
// Timer到期回调函数类型
typedef std::function<void()> TimerCallback;
// 
// typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;
// 
// typedef std::function<void(const TcpConnectionPtr&)> CloseCallback;
// 
// typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
// 
// typedef std::function<void(const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;
// 
// typedef std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)> MessageCallback;

// 
// void defaultConnectionCallback(const TcpConnectionPtr& conn);
// 
// void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime);

} // end namespace net

} // end namespace sznet

#endif // _SZNET_NET_CALLBACKS_H_
