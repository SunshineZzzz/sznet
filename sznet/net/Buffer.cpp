#include "Buffer.h"
#include "../process/Process.h"
#include "UdpOps.h"
#include "InetAddress.h"

namespace sznet
{

namespace net
{

const char Buffer::kCRLF[] = "\r\n";

sz_ssize_t Buffer::readvFd(sockets::sz_sock fd, int* savedErrno)
{
	// �ж��Ƿ���Ҫ�ص�
	if ((begin() + m_writerIndex) >= (begin() + m_wrapSize))
	{
		wrapBuffer();
	}
	// ���ٵ�ջ�ռ�
	char extrabuf[65536];
	net::sockets::sz_iov_type vec[2];
	// ������������д������ֽ�
	const size_t writable = writableBytes();
	sz_set_iov_buf(vec[0], (begin() + m_writerIndex));
	sz_set_iov_buflen(vec[0], writable);
	sz_set_iov_buf(vec[1], extrabuf);
	sz_set_iov_buflen(vec[1], sizeof(extrabuf));
	// ���Ӧ�ò���������㹻�󣬾Ͳ���Ҫ��ջ��д������
	const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
	const sz_ssize_t n = sockets::sz_readv(fd, vec, iovcnt);
	if (n < 0)
	{
		*savedErrno = sz_getlasterr();
	}
	// ˵��û�õ�extrabuf
	else if (implicit_cast<size_t>(n) <= writable)
	{
		m_writerIndex += n;
	}
	// �õ�extrabuf
	else
	{
		m_writerIndex = m_buffer.size();
		append(extrabuf, n - writable);
	}

	return n;
}

sz_ssize_t Buffer::readFd(sockets::sz_sock fd, int* savedErrno)
{
	// �ж��Ƿ���Ҫ�ص�
	if ((begin() + m_writerIndex) >= (begin() + m_wrapSize))
	{
		wrapBuffer();
	}

	char* buf = (begin() + m_writerIndex);
	const size_t writable = writableBytes();
	const sz_ssize_t n = sockets::sz_read(fd, buf, static_cast<int>(writable));
	if (n < 0)
	{
		*savedErrno = sz_getlasterr();
	}
	else
	{
		m_writerIndex += n;
	}
	
	return n;
}

sz_ssize_t Buffer::recvFrom(sockets::sz_sock fd, struct sockaddr* addr, int* savedErrno)
{
	// �ж��Ƿ���Ҫ�ص�
	if ((begin() + m_writerIndex) >= (begin() + m_wrapSize))
	{
		wrapBuffer();
	}
	
	char* buf = (begin() + m_writerIndex);
	const size_t writable = writableBytes();
	const sz_ssize_t n = sockets::sz_udp_recv(fd, buf, static_cast<int>(writable), addr);
	if (n < 0)
	{
		*savedErrno = sz_getlasterr();
	}
	else
	{
		m_writerIndex += n;
	}

	return n;
}

} // end namespace net

} // end namespace sznet
