// Comment: 弱回调实现

#ifndef _SZNET_BASE_WEAKCALLBACK_H_
#define _SZNET_BASE_WEAKCALLBACK_H_

#include <functional>
#include <memory>

namespace sznet
{

// 弱回调
template<typename CLASS, typename... ARGS>
class WeakCallback
{
public:
	WeakCallback(const std::weak_ptr<CLASS>& object, const std::function<void(CLASS*, ARGS...)>& function): 
		m_object(object),
		m_function(function)
	{
	}

	// 弱回调
	void operator()(ARGS&&... args) const
	{
		// 看看是否空悬
		std::shared_ptr<CLASS> ptr(m_object.lock());
		if (ptr)
		{
			// 没有空悬就调用回调函数
			m_function(ptr.get(), std::forward<ARGS>(args)...);
		}
		// else
		// {
		//   LOG_TRACE << "expired";
		// }
	}

private:
	// 弱对象
	std::weak_ptr<CLASS> m_object;
	// 回调函数
	std::function<void(CLASS*, ARGS...)> m_function;
};

// 生成弱回调对象
template<typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> makeWeakCallback(const std::shared_ptr<CLASS>& object, void (CLASS::*function)(ARGS...))
{
	return WeakCallback<CLASS, ARGS...>(object, function);
}
template<typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> makeWeakCallback(const std::shared_ptr<CLASS>& object, void (CLASS::*function)(ARGS...) const)
{
	return WeakCallback<CLASS, ARGS...>(object, function);
}

} // namespace sznet

#endif // _SZNET_BASE_WEAKCALLBACK_H_
