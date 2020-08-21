#include "FileUtil.h"
#include "../log/Logging.h"
#include "IO.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <algorithm>

namespace sznet
{

// 'e' for O_CLOEXEC => close-on-exec => 句柄在fork子进程后执行exec时就关闭
FileUtil::AppendFile::AppendFile(StringArg filename):
#ifdef SZ_OS_LINUX
	m_fp(sz_fopen(filename.c_str(), "ae")),
#else
	m_fp(sz_fopen(filename.c_str(), "a+")),
#endif
	m_writtenBytes(0)
{
	assert(m_fp);
	// 设置文件指针m_fp的缓冲区设定64K，也就是文件的stream大小
	// _IOFBF 全缓存
	::setvbuf(m_fp, m_buffer, _IOFBF, sizeof(m_buffer));
}

FileUtil::AppendFile::~AppendFile()
{
	sz_fclose(m_fp);
}

void FileUtil::AppendFile::append(const char* logline, const size_t len)
{
	// 返回的n是已经写入文件的字节数
	size_t n = write(logline, len);
	// 相减大于0表示未写完
	size_t remain = len - n;
	while (remain > 0)
	{
		// 同样x是已经写的字节数
		size_t x = write(logline + n, remain);
		if (x == 0)
		{
			int err = ferror(m_fp);
			if (err)
			{
				fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));
			}
			break;
		}
		// 偏移
		n += x;
		remain = len - n;
	}
	m_writtenBytes += len;
}

void FileUtil::AppendFile::flush()
{
	sz_fflush(m_fp);
}

size_t FileUtil::AppendFile::write(const char* logline, size_t len)
{
	// 不加锁的方式写入，效率高，not thread safe
	return sz_fwrite_unlocked(logline, 1, len, m_fp);
}

FileUtil::ReadSmallFile::ReadSmallFile(StringArg filename) :
#ifdef SZ_OS_LINUX
	m_fd(sz_open(filename.c_str(), O_RDONLY | O_CLOEXEC)),
#else
	m_fd(sz_open(filename.c_str(), O_RDONLY)),
#endif
	m_err(0)
{
	m_buf[0] = '\0';
	if (m_fd < 0)
	{
		m_err = errno;
	}
}

FileUtil::ReadSmallFile::~ReadSmallFile()
{
	if (m_fd >= 0)
	{
		sz_close(m_fd);
	}
}

template<typename String>
int FileUtil::ReadSmallFile::readToString(int maxSize, String* content, int64_t* fileSize, int64_t* modifyTime, int64_t* createTime)
{
	assert(content != nullptr);
	int err = m_err;
	if (m_fd >= 0)
	{
		content->clear();
		// 如果不为空，获取文件大小
		if (fileSize)
		{
			struct stat statbuf;
			// fstat函数用来获取文件的属性，保存到缓冲区当中
			if (sz_fstat(m_fd, &statbuf) == 0)
			{
				if (S_ISREG(statbuf.st_mode))
				{
					*fileSize = statbuf.st_size;
					content->reserve(static_cast<int>(std::min(implicit_cast<int64_t>(maxSize), *fileSize)));
				}
				else if (S_ISDIR(statbuf.st_mode))
				{
					err = EISDIR;
				}
				if (modifyTime)
				{
					*modifyTime = statbuf.st_mtime;
				}
				if (createTime)
				{
					*createTime = statbuf.st_ctime;
				}
			}
			else
			{
				err = errno;
			}
		}

		// 没有写到maxSize，就反复写
		while (content->size() < implicit_cast<size_t>(maxSize))
		{
			// 这次允许读取的最多数量
			size_t toRead = std::min(implicit_cast<size_t>(maxSize) - content->size(), sizeof(m_buf));
			size_t n = sz_read(m_fd, m_buf, toRead);
			if (n > 0)
			{
				content->append(m_buf, n);
			}
			else
			{
				// 结尾或者出错，都停止
				if (n < 0)
				{
					err = errno;
				}
				break;
			}
		}
	}
	return err;
}

int FileUtil::ReadSmallFile::readToBuffer(int* size)
{
	int err = m_err;
	if (m_fd >= 0)
	{
		// pread, pwrite - read from or write to a file descriptor at a given offset
		// pread和read区别，pread读取完文件offset不会更改，而read会引发offset随读到的字节数移动
		// 返回已读的字节数
		// ssize_t n = ::pread(fd_, buf_, sizeof(buf_) - 1, 0);
		size_t n = sz_read(m_fd, m_buf, sizeof(m_buf) - 1);
		if (n >= 0)
		{
			if (size)
			{
				*size = static_cast<int>(n);
			}
			m_buf[n] = '\0';
		}
		else
		{
			err = errno;
		}
	}
	return err;
}

// explicit instantiation
template int FileUtil::readFile(StringArg filename, int maxSize, string* content, int64_t*, int64_t*, int64_t*);
template int FileUtil::ReadSmallFile::readToString(int maxSize, string* content, int64_t*, int64_t*, int64_t*);

} // end namespace sznet