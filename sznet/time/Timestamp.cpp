#include "Timestamp.h"
#include "Time.h"

namespace sznet
{

static_assert(sizeof(Timestamp) == sizeof(int64_t),
	"Timestamp should be same size as int64_t");

Timestamp::Timestamp() :
	m_microSecondsSinceEpoch(0)
{
}

Timestamp::Timestamp(int64_t microSecondsSinceEpochArg) :
	m_microSecondsSinceEpoch(microSecondsSinceEpochArg)
{
}

void Timestamp::swap(Timestamp& that)
{
	std::swap(m_microSecondsSinceEpoch, that.m_microSecondsSinceEpoch);
}

string Timestamp::toString() const
{
	char buf[32] = { 0 };
	int64_t seconds = m_microSecondsSinceEpoch / kMicroSecondsPerSecond;
	int64_t microseconds = m_microSecondsSinceEpoch % kMicroSecondsPerSecond;
	snprintf(buf, sizeof(buf), "%" PRId64 "d.%06" PRId64 "d", seconds, microseconds);
	return buf;
}

string Timestamp::toFormattedString(bool showMicroseconds) const
{
	char buf[32] = { 0 };
	time_t seconds = static_cast<time_t>(m_microSecondsSinceEpoch / kMicroSecondsPerSecond);
	struct tm tm_time;
	sz_localtime(tm_time, seconds);

	if (showMicroseconds)
	{
		int microseconds = static_cast<int>(m_microSecondsSinceEpoch % kMicroSecondsPerSecond);
		snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
			tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
			tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
			microseconds);
	}
	else
	{
		snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
			tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
			tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
	}

	return buf;
}

bool Timestamp::valid() const
{
	return m_microSecondsSinceEpoch > 0;
}

int64_t Timestamp::microSecondsSinceEpoch() const
{
	return m_microSecondsSinceEpoch;
}

time_t Timestamp::secondsSinceEpoch() const
{
	return static_cast<time_t>(m_microSecondsSinceEpoch / kMicroSecondsPerSecond);
}

Timestamp Timestamp::now()
{
	struct timeval tv;
	sz_gettimeofday(&tv);
	int64_t seconds = tv.tv_sec;
	return Timestamp(seconds * Timestamp::kMicroSecondsPerSecond + tv.tv_usec);
}

Timestamp Timestamp::invalid()
{
	return Timestamp();
}

Timestamp Timestamp::fromUnixTime(time_t t)
{
	return fromUnixTime(t, 0);
}

Timestamp Timestamp::fromUnixTime(time_t t, int microseconds)
{
	return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
}

} // end namespace sznet