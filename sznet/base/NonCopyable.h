// Comment: 不可拷贝基类实现

#ifndef _SZNET_BASE_NONCOPYABLE_H_
#define _SZNET_BASE_NONCOPYABLE_H_

namespace sznet
{

class NonCopyable
{
public:
	NonCopyable(const NonCopyable&) = delete;
	void operator=(const NonCopyable&) = delete;

protected:
	NonCopyable() = default;
	~NonCopyable() = default;
};

} // end namespace sznet

#endif // _SZNET_BASE_NONCOPYABLE_H_