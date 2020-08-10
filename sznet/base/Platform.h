// From https://github.com/google/googletest/blob/master/googletest/include/gtest/internal/gtest-port-arch.h

#ifndef _SZNET_BASE_PLATFORM_H_
#define _SZNET_BASE_PLATFORM_H_

// Determines the platform on which Google Test is compiled.
#ifdef __CYGWIN__
#	define SZ_OS_CYGWIN 1
#elif defined(__MINGW__) || defined(__MINGW32__) || defined(__MINGW64__)
#	define SZ_OS_WINDOWS_MINGW 1
#	define SZ_OS_WINDOWS 1
#elif defined _WIN32
#	define SZ_OS_WINDOWS 1
#	ifdef _WIN32_WCE
#		define SZ_OS_WINDOWS_MOBILE 1
#	elif defined(WINAPI_FAMILY)
#		include <winapifamily.h>
#		if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#			define SZ_OS_WINDOWS_DESKTOP 1
#		elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
#			define SZ_OS_WINDOWS_PHONE 1
#		elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
#			define SZ_OS_WINDOWS_RT 1
#		elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_TV_TITLE)
#			define SZ_OS_WINDOWS_PHONE 1
#			define SZ_OS_WINDOWS_TV_TITLE 1
#		else
//			WINAPI_FAMILY defined but no known partition matched.
//			Default to desktop.
#			define SZ_OS_WINDOWS_DESKTOP 1
#		endif
#	else
#		define SZ_OS_WINDOWS_DESKTOP 1
#	endif // _WIN32_WCE
#elif defined __OS2__
#	define SZ_OS_OS2 1
#elif defined __APPLE__
#	define SZ_OS_MAC 1
#	if TARGET_OS_IPHONE
#		define SZ_OS_IOS 1
#	endif
#elif defined __DragonFly__
#	define SZ_OS_DRAGONFLY 1
#elif defined __FreeBSD__
#	define SZ_OS_FREEBSD 1
#elif defined __Fuchsia__
#	define SZ_OS_FUCHSIA 1
#elif defined(__GLIBC__) && defined(__FreeBSD_kernel__)
#	define SZ_OS_GNU_KFREEBSD 1
#elif defined __linux__
#	define SZ_OS_LINUX 1
#	if defined __ANDROID__
#		define SZ_OS_LINUX_ANDROID 1
#	endif
#elif defined __MVS__
#	define SZ_OS_ZOS 1
#elif defined(__sun) && defined(__SVR4)
#	define SZ_OS_SOLARIS 1
#elif defined(_AIX)
#	define SZ_OS_AIX 1
#elif defined(__hpux)
#	define SZ_OS_HPUX 1
#elif defined __native_client__
#	define SZ_OS_NACL 1
#elif defined __NetBSD__
#	define SZ_OS_NETBSD 1
#elif defined __OpenBSD__
#	define SZ_OS_OPENBSD 1
#elif defined __QNX__
#	define SZ_OS_QNX 1
#elif defined(__HAIKU__)
#	define SZ_OS_HAIKU 1
#elif defined ESP8266
#	define SZ_OS_ESP8266 1
#elif defined ESP32
#	define SZ_OS_ESP32 1
#endif // __CYGWIN__

#endif // _SZNET_BASE_PLATFORM_H_// 