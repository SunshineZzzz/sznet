#ifndef _SZNET_IO_FILEUTIL_H_
#define _SZNET_IO_FILEUTIL_H_

#include "../base/NonCopyable.h"
#include "../string/StringPiece.h"

namespace sznet
{

namespace FileUtil
{

// RAII
// 读取小文件(小于64KB)
class ReadSmallFile : NonCopyable
{
public:
	ReadSmallFile(StringArg filename);
	~ReadSmallFile();

	// 该函数用于将小文件的内容转换为字符串存入content
	template<typename String>
	int readToString(
		int maxSize, // 期望读取的最大的大小
		String* content, // 要读入的content缓冲区
		int64_t* fileSize, // 读取出的整个文件大小
		int64_t* modifyTime, // 读取出的文件修改的时间
		int64_t* createTime); // 读取出的创建文件的时间
	// 从文件读取数据到m_buf
	int readToBuffer(int* size);
	// 返回buffer指针
	const char* buffer() const 
	{	
		return m_buf; 
	}
	// buf最大大小
	static const int kBufferSize = 64 * 1024;

private:
	// 文件fd
	int m_fd;
	// 文件如果打开失败的错误号
	int m_err;
	// buffer
	char m_buf[kBufferSize];
};

// 一个全局函数，readFile函数，调用ReadSmallFile类中的readToString方法，供外部将文件中的内容转化为字符串。
template<typename String>
int readFile(StringArg filename, int maxSize, String* content, int64_t* fileSize = nullptr, int64_t* modifyTime = nullptr, int64_t* createTime = nullptr)
{
	ReadSmallFile file(filename);
	return file.readToString(maxSize, content, fileSize, modifyTime, createTime);
}

// 封装了一个文件指针的操作类，用于把数据写入文件中
// 非线程安全
class AppendFile : NonCopyable
{
public:
	explicit AppendFile(StringArg filename);
	~AppendFile();

	// 追加,不是线程安全的，需要外部加锁
	void append(const char* logline, size_t len);
	// 刷新流
	void flush();
	// 返回已经写入字节个数
	size_t writtenBytes() const
	{ 
		return m_writtenBytes;
	}

private:
	// 向fp中写入数据，使用非线程安全的fwrite_unlocked
	size_t write(const char* logline, size_t len);
	// 文件fp
	FILE* m_fp;
	// 写入流缓冲区
	char m_buffer[64 * 1024];
	// 已经写入的字节个数
	size_t m_writtenBytes;
};

} // end namespace FileUtil

} // end namespace sznet

#endif // _SZNET_IO_FILEUTIL_H_

