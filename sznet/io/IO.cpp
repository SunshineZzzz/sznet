#include "IO.h"

namespace sznet
{

FILE* sz_fopen(const char* name, const char* mode)
{
	return fopen(name, mode);
}

int sz_fclose(FILE* s)
{
	return fclose(s);
}

int sz_fflush(FILE* s)
{
	return fflush(s);
}

size_t sz_fwrite_unlocked(const void* ptr, size_t size, size_t n, FILE* stream)
{
#if defined(SZ_OS_WINDOWS)
	return _fwrite_nolock(ptr, size, n, stream);
#elif defined(SZ_OS_LINUX)
	return fwrite_unlocked(ptr, size, n, stream);
#endif
}

int sz_open(const char* pathname, int flags)
{
#if defined(SZ_OS_WINDOWS)
	return _open(pathname, flags);
#elif defined(SZ_OS_LINUX)
	return open(pathname, flags);
#endif
}

int sz_close(int fd)
{
#if defined(SZ_OS_WINDOWS)
	return _close(fd);
#elif defined(SZ_OS_LINUX)
	return close(fd);
#endif
}

int sz_read(int fd, void* buf, size_t nbyte)
{
#if defined(SZ_OS_WINDOWS)
	if (nbyte > UINT32_MAX)
	{
		assert(0);
	}
	unsigned int count = static_cast<unsigned int>(nbyte);
	return _read(fd, buf, count);
#elif defined(SZ_OS_LINUX)
	return read(fd, buf, nbyte);
#endif
}

int sz_fstat(int fd, struct stat* const stat)
{
	return fstat(fd, stat);
}

} // end namespace sznet