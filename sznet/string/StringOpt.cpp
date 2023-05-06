#include "StringOpt.h"

namespace sznet
{

void sz_split(const sznet::string& s, std::vector<sznet::StringPiece>& tokens, const sznet::string& delimiters)
{
	sznet::string::size_type beginPos = s.find_first_not_of(delimiters, 0);
	sznet::string::size_type endPos = s.find_first_of(delimiters, beginPos);
	size_t size = 0;
	while (sznet::string::npos != endPos || sznet::string::npos != beginPos)
	{
		size = endPos - beginPos;
		if (endPos == sznet::string::npos)
		{
			size = s.size() - beginPos;
		}
		// assert(size <= std::numeric_limits<int>::max());
		tokens.emplace_back(s.data() + beginPos, static_cast<int>(size));
		beginPos = s.find_first_not_of(delimiters, endPos);
		endPos = s.find_first_of(delimiters, beginPos);
	}
}

} // end namespace sznet