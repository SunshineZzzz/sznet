// From https://sourceforge.net/p/predef/wiki/Compilers/

#ifndef _SZNET_BASE_COMPILER_H_
#define _SZNET_BASE_COMPILER_H_

#ifdef __GNUC__
#	define SZ_COMPILER_GNUC 1
//	40805 means version 4.8.5
#	define SZ_COMPILER_GNUC_VER (__GNUC__*10000 + __GNUC_MINOR__*100 + __GNUC_PATCHLEVEL__)
#elif defined __clang__
#	define SZ_COMPILER_CLANG 1
//	30402 means version 3.4.2
#	define SZ_COMPILER_CLANG_VER (__clang_major__*10000 + __clang_minor__*100 + __clang_patchlevel__)
#elif defined _MSC_VER
#	define SZ_COMPILER_MSVC 1
//	@see https://en.wikipedia.org/wiki/Microsoft_Visual_C%2B%2B
#	define SZ_COMPILER_MSVC_VER _MSC_VER
#endif // __GNUC__

#endif // _SZNET_BASE_COMPILER_H_