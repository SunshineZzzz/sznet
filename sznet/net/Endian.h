// From: https://gist.github.com/nurupo/9893e2b8fa70f454455c
//     : https://android.googlesource.com/platform/bionic/+/master/libc/include/sys/endian.h
// Comment: 网络字节序相关函数接口

#ifndef _SZNET_NET_ENDIAN_H_
#define _SZNET_NET_ENDIAN_H_

#include "../NetCmn.h"

#ifdef SZ_OS_LINUX
#	include <endian.h>
#endif

#ifdef SZ_OS_WINDOWS

#	ifndef LITTLE_ENDIAN
#		define LITTLE_ENDIAN 1234
#	endif
#	ifndef BIG_ENDIAN
#		define BIG_ENDIAN 4321
#	endif
#	ifndef BYTE_ORDER
#		define BYTE_ORDER LITTLE_ENDIAN
#	endif

#	if BYTE_ORDER == LITTLE_ENDIAN

#		define htobe16(x) htons(x)
#		define htole16(x) (x)
#		define be16toh(x) ntohs(x)
#		define le16toh(x) (x)

#		define htobe32(x) htonl(x)
#		define htole32(x) (x)
#		define be32toh(x) ntohl(x)
#		define le32toh(x) (x)

#		define htobe64(x) htonll(x)
#		define htole64(x) (x)
#		define be64toh(x) ntohll(x)
#		define le64toh(x) (x)

#	elif BYTE_ORDER == BIG_ENDIAN

		/* that would be xbox 360 */
#		define htobe16(x) (x)
#		define htole16(x) __builtin_bswap16(x)
#		define be16toh(x) (x)
#		define le16toh(x) __builtin_bswap16(x)

#		define htobe32(x) (x)
#		define htole32(x) __builtin_bswap32(x)
#		define be32toh(x) (x)
#		define le32toh(x) __builtin_bswap32(x)

#		define htobe64(x) (x)
#		define htole64(x) __builtin_bswap64(x)
#		define be64toh(x) (x)
#		define le64toh(x) __builtin_bswap64(x)

#	else

#		error byte order not supported

#	endif

#endif

namespace sznet
{

namespace net
{

namespace sockets
{

// 无符号64位主机序转网络字节序
inline uint64_t hostToNetwork64(uint64_t host64)
{
	return htobe64(host64);
}

// 无符号32位主机序转网络字节序
inline uint32_t hostToNetwork32(uint32_t host32)
{
	return htobe32(host32);
}

// 无符号16位主机序转网络字节序
inline uint16_t hostToNetwork16(uint16_t host16)
{
	return htobe16(host16);
}

// 无符号62位网络字节序转主机序
inline uint64_t networkToHost64(uint64_t net64)
{
	return be64toh(net64);
}

// 无符号32位网络字节序转主机序 
inline uint32_t networkToHost32(uint32_t net32)
{
	return be32toh(net32);
}

// 无符号16位网络字节序转主机序 
inline uint16_t networkToHost16(uint16_t net16)
{
	return be16toh(net16);
}

} // end namespace sockets

} // end namespace net

}  // end namespace sznet

#endif // _SZNET_NET_ENDIAN_H_
