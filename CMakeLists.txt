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
	# -Wextra
	# -Werror
	# -Wconversion
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

aux_source_directory("sznet/." SRC_LIST)

aux_source_directory("sznet/." SRC_LIST)
aux_source_directory("sznet/base" SRC_LIST)
aux_source_directory("sznet/ds" SRC_LIST)
aux_source_directory("sznet/io" SRC_LIST)
aux_source_directory("sznet/log" SRC_LIST)
aux_source_directory("sznet/net" SRC_LIST)
aux_source_directory("sznet/process" SRC_LIST)
aux_source_directory("sznet/string" SRC_LIST)
aux_source_directory("sznet/thread" SRC_LIST)
aux_source_directory("sznet/time" SRC_LIST)

add_library(${PROJECT_NAME} STATIC ${SRC_LIST})

if(SZNET_BUILD_EXAMPLES)
	add_subdirectory(test)
endif()