#ifndef _SZNET_TIME_TIMESTAMP_H_
#define _SZNET_TIME_TIMESTAMP_H_

#include <string>

#include "../NetCmn.h"
#include "../base/Copyable.h"
#include "../base/Types.h"

namespace sznet
{

class Timestamp : public Copyable
{
public:
	Timestamp();
	explicit Timestamp(int64_t microSecondsSinceEpochArg);

	// 交换函数
	void swap(Timestamp& that);
	// 返回 秒.xxxxxx(微妙)
	string toString() const;
	// 转换成一个格式化字符串 .[xxxxxx(微妙)]
	string toFormattedString(bool showMicroseconds = true) const;
	// 大于0有效
	bool valid() const;
	// 返回从1970年到当前的时间，单位微秒
	int64_t microSecondsSinceEpoch() const;
	// 从1970年到当前的秒
	time_t secondsSinceEpoch() const;
	// 获取当前的时间戳
	static Timestamp now();
	// 获取一个无效时间，即时间等于0
	static Timestamp invalid();
	// 从unixtime来生成
	static Timestamp fromUnixTime(time_t t);
	// 从unixtime来生成
	static Timestamp fromUnixTime(time_t t, int microseconds);
	// 每秒等于多少微秒
	static const int kMicroSecondsPerSecond = 1000 * 1000;
	// 每毫秒等于多少微秒
	static const int kMicroSecondsPerMillisecond = 1000;

private:
	// 从1970年到当前的时间，单位微秒
	int64_t m_microSecondsSinceEpoch;
};

// 小于
inline bool operator<(Timestamp lhs, Timestamp rhs)
{
	return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}
// 判等
inline bool operator==(Timestamp lhs, Timestamp rhs)
{
	return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}
// 实现两个时间的差S
inline double timeDifference(Timestamp high, Timestamp low)
{
	int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
	return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}
// 实现两个时间的差MS
inline int timeDifferenceMs(Timestamp high, Timestamp low)
{
	int64_t diff = static_cast<int64_t>(high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch());
	return static_cast<int>(diff / Timestamp::kMicroSecondsPerMillisecond);
}

// 时间戳基础上加上指定的秒
inline Timestamp addTime(Timestamp timestamp, double seconds)
{
	int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
	return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

}  // end namespace sznet

#endif // _SZNET_TIME_TIMESTAMP_H_