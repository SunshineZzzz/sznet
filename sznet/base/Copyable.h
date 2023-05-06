// Comment: 可拷贝基类的实现

#ifndef _SZNET_BASE_COPYABLE_H_
#define _SZNET_BASE_COPYABLE_H_

namespace sznet
{

// 可拷贝基类
class Copyable
{
protected:
	Copyable() = default;
	~Copyable() = default;
};

} // end namespace sznet

#endif // _SZNET_BASE_COPYABLE_H_