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
#else
	return fwrite_unlocked(ptr, size, n, stream);
#endif
}

int sz_open(const char* pathname, int flags)
{
#if defined(SZ_OS_WINDOWS)
	return _open(pathname, flags);
#else
	return open(pathname, flags);
#endif
}

int sz_close(int fd)
{
#if defined(SZ_OS_WINDOWS)
	return _close(fd);
#else
	return close(fd);
#endif
}

sz_ssize_t sz_read(int fd, void* buf, size_t nbyte)
{
#if defined(SZ_OS_WINDOWS)
	if (unlikely(nbyte > std::numeric_limits<unsigned int>::max()))
	{
		assert(0);
	}
	unsigned int count = static_cast<unsigned int>(nbyte);
	return _read(fd, buf, count);
#else
	return read(fd, buf, nbyte);
#endif
}

int sz_fstat(int fd, struct stat* const stat)
{
	return fstat(fd, stat);
}

int sz_fdel(const char* name)
{
#if defined(SZ_OS_WINDOWS)
	return _unlink(name);
#else
	return unlink(name);
#endif
}

} // end namespace sznet