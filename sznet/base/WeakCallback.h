// Comment: ���ص�ʵ��

#ifndef _SZNET_BASE_WEAKCALLBACK_H_
#define _SZNET_BASE_WEAKCALLBACK_H_

#include <functional>
#include <memory>

namespace sznet
{

// ���ص�
template<typename CLASS, typename... ARGS>
class WeakCallback
{
public:
	WeakCallback(const std::weak_ptr<CLASS>& object, const std::function<void(CLASS*, ARGS...)>& function): 
		m_object(object),
		m_function(function)
	{
	}

	// ���ص�
	void operator()(ARGS&&... args) const
	{
		// �����Ƿ����
		std::shared_ptr<CLASS> ptr(m_object.lock());
		if (ptr)
		{
			// û�п����͵��ûص�����
			m_function(ptr.get(), std::forward<ARGS>(args)...);
		}
		// else
		// {
		//   LOG_TRACE << "expired";
		// }
	}

private:
	// ������
	std::weak_ptr<CLASS> m_object;
	// �ص�����
	std::function<void(CLASS*, ARGS...)> m_function;
};

// �������ص�����
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
