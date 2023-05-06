cmake_minimum_required(VERSION 2.6)

project(sznet C CXX)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_PROJECT_NAME STREQUAL "sznet")
	option(SZNET_BUILD_EXAMPLES "Build sznet examples" ON)
endif()

set(CXX_FLAGS
	-g
	# -DVALGRIND
	-DCHECK_PTHREAD_RETURN_VALUE
	-D_FILE_OFFSET_BITS=64
	-Wall
	-Wextra
	-Werror
	-Wconversion
	-Wno-unused-parameter
	-Wold-style-cast
	-Woverloaded-virtual
	-Wpointer-arith
	-Wshadow
	-Wwrite-strings
	-march=native
	# -MMD
	-std=c++11
	-m64
	-rdynamic
)
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR})

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

add_subdirectory(sznet)
add_subdirectory(sznet/base)
add_subdirectory(sznet/ds)
add_subdirectory(sznet/io)
add_subdirectory(sznet/log)
add_subdirectory(sznet/memory)
add_subdirectory(sznet/net)
add_subdirectory(sznet/process)
add_subdirectory(sznet/string)
add_subdirectory(sznet/thread)
add_subdirectory(sznet/time)

if(SZNET_BUILD_EXAMPLES)
	# add_subdirectory(test)
endif()

