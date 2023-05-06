// Comment: string view实现

// Taken from PCRE pcre_stringpiece.h
//
// Copyright (c) 2005, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Sanjay Ghemawat
//
// A string like object that points into another piece of memory.
// Useful for providing an interface that allows clients to easily
// pass in either a "const char*" or a "string".
//
// Arghh!  I wish C++ literals were automatically of type "string".

#ifndef _SZNET_STRING_STRINGPIECE_H_
#define _SZNET_STRING_STRINGPIECE_H_

#include "../NetCmn.h"
#include <string>
#include <string.h>
// for ostream forward-declaration
#include <iosfwd>

namespace sznet
{

// 用于传递c风格的字符串给函数
// For passing C-style string argument to a function.
class StringArg // copyable
{
public:
	StringArg(const char* str) :
		m_str(str)
	{
	}
	StringArg(const string& str) :
		m_str(str.c_str())
	{
	}

	const char* c_str() const
	{
		return m_str;
	}

private:
	const char* m_str;
}; // end class StringArg

	// string_view
	// 不涉及内存拷贝
class StringPiece
{
private:
	// 字符指针
	const char* m_ptr;
	// 字符串长度
	int m_length;

public:
	// We provide non-explicit singleton constructors so users can pass
	// in a "const char*" or a "string" wherever a "StringPiece" is
	// expected.
	StringPiece() :
		m_ptr(nullptr),
		m_length(0)
	{
	}
	StringPiece(const char* str) :
		m_ptr(str),
		m_length(static_cast<int>(strlen(m_ptr)))
	{
	}
	StringPiece(const unsigned char* str) :
		m_ptr(reinterpret_cast<const char*>(str)),
		m_length(static_cast<int>(strlen(m_ptr)))
	{
	}
	StringPiece(const string& str) :
		m_ptr(str.data()),
		m_length(static_cast<int>(str.size()))
	{
	}
	StringPiece(const char* offset, int len) :
		m_ptr(offset),
		m_length(len)
	{
	}

	// data() may return a pointer to a buffer with embedded NULs, and the
	// returned buffer may or may not be null terminated.  Therefore it is
	// typically a mistake to pass data() to a routine that expects a NUL
	// terminated string.  Use "as_string().c_str()" if you really need to do
	// this.  Or better yet, change your routine so it does not rely on NUL
	// termination.
	// 返回字符串指针 
	const char* data() const
	{
		return m_ptr;
	}
	// 返回字符串长度
	int size() const
	{
		return m_length;
	}
	// 判断是否为空
	bool empty() const
	{
		return m_length == 0;
	}
	// 起始指针
	const char* begin() const
	{
		return m_ptr;
	}
	// 终止指针
	const char* end() const
	{
		return m_ptr + m_length;
	}
	// 清空
	void clear()
	{
		m_ptr = nullptr;
		m_length = 0;
	}
	// 重置
	void set(const char* buffer, int len)
	{
		m_ptr = buffer;
		m_length = len;
	}
	void set(const char* str)
	{
		m_ptr = str;
		m_length = static_cast<int>(strlen(str));
	}
	void set(const void* buffer, int len)
	{
		m_ptr = reinterpret_cast<const char*>(buffer);
		m_length = len;
	}
	// 重载[]运算符
	char operator[](int i) const
	{
		return m_ptr[i];
	}
	// 截掉前n个字符
	void remove_prefix(int n)
	{
		m_ptr += n;
		m_length -= n;
	}
	// 截掉后n个字符
	void remove_suffix(int n)
	{
		m_length -= n;
	}
	// 重载==运算符
	bool operator==(const StringPiece& x) const
	{
		return ((m_length == x.m_length) &&
			(memcmp(m_ptr, x.m_ptr, m_length) == 0));
	}
	// 重载!=运算符
	bool operator!=(const StringPiece& x) const
	{
		return !(*this == x);
	}
	// 辅助生成运算符重载
#define STRINGPIECE_BINARY_PREDICATE(cmp,auxcmp)										\
	bool operator cmp (const StringPiece& x) const										\
	{																					\
		int r = memcmp(m_ptr, x.m_ptr, m_length < x.m_length ? m_length : x.m_length);  \
		return ((r auxcmp 0) || ((r == 0) && (m_length cmp x.m_length)));				\
	}
	// 小于运算符
	STRINGPIECE_BINARY_PREDICATE(<, <);
	// 小于等于运算符
	STRINGPIECE_BINARY_PREDICATE(<= , <);
	// 大于等于运算符
	STRINGPIECE_BINARY_PREDICATE(>= , >);
	// 大于运算符
	STRINGPIECE_BINARY_PREDICATE(>, >);
#undef STRINGPIECE_BINARY_PREDICATE
	// 比较两个对象是否相等，利用memcmp进行比较
	int compare(const StringPiece& x) const
	{
		int r = memcmp(m_ptr, x.m_ptr, m_length < x.m_length ? m_length : x.m_length);
		if (r == 0)
		{
			if (m_length < x.m_length) r = -1;
			else if (m_length > x.m_length) r = +1;
		}
		return r;
	}
	// 以 string 类型返回
	string as_string() const
	{
		return string(data(), size());
	}
	// 将string内容清空，并将ptr_字符串赋值给string
	void CopyToString(string* target) const
	{
		target->assign(m_ptr, m_length);
	}
	// 将StringPiece与当前字符串比较是否相同
	// Does "this" start with "x"
	bool starts_with(const StringPiece& x) const
	{
		return ((m_length >= x.m_length) && (memcmp(m_ptr, x.m_ptr, x.m_length) == 0));
	}
};

} // end namespace sznet

// ------------------------------------------------------------------
// Functions used to create STL containers that use StringPiece
//  Remember that a StringPiece's lifetime had better be less than
//  that of the underlying string or char*.  If it is not, then you
//  cannot safely store a StringPiece into an STL container
// ------------------------------------------------------------------

#ifdef HAVE_TYPE_TRAITS
  // 经过特化的StringPiece 
  // This makes vector<StringPiece> really fast for some STL implementations
template<> struct __type_traits<sznet::StringPiece> {
	// 有构造函数
	typedef __true_type    has_trivial_default_constructor;
	// 有复制构造函数
	typedef __true_type    has_trivial_copy_constructor;
	// 有赋值函数
	typedef __true_type    has_trivial_assignment_operator;
	// 有析构函数
	typedef __true_type    has_trivial_destructor;
	// 是C++的内建类型或传统的C结构体类型
	typedef __true_type    is_POD_type;
};
#endif

// allow StringPiece to be logged
std::ostream& operator<<(std::ostream& o, const sznet::StringPiece& piece);

#endif  // _SZNET_STRING_STRINGPIECE_H_
