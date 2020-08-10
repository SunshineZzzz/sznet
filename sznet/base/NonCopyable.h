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

} // namespace sznet

#endif // _SZNET_BASE_NONCOPYABLE_H_