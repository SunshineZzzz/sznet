// Comment: �ַ�����ز���

#ifndef _SZNET_STRINGOPT_H_
#define _SZNET_STRINGOPT_H_

#include "../NetCmn.h"
#include "StringPiece.h"

#include <vector>

namespace sznet
{

// �ַ����ָ�
void sz_split(const sznet::string& s, std::vector<sznet::StringPiece>& tokens, const sznet::string& delimiters = " ");

} // end namespace sznet

#endif // _SZNET_STRINGOPT_H_
