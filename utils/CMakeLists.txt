#CMakeList to make media_filter lib

cmake_minimum_required(VERSION 2.6)
project(utils)
#The version number
set(utils_VERSION_MAJOR 1)
set(utils_VERSION_MINOR 0)

#configure a header file to pass some of the CMake settings
#to the source code
configure_file(
    "${PROJECT_SOURCE_DIR}/utils_cfg.h.in"
    "${PROJECT_BINARY_DIR}/utils_cfg.h"
    )

set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(INC_DIR ${PROJECT_SOURCE_DIR}/inc)

set(BOOST_ROOT ${PROJECT_SOURCE_DIR}/../3rd/boost_1_68_0)
set(BOOST_INCLUDEDIR ${PROJECT_SOURCE_DIR}/../3rd/boost_1_68_0/include)
set(BOOST_LIBARARYDIR ${PROJECT_SOURCE_DIR}/../3rd/boost_1_68_0/lib)
include_directories("${BOOST_INCLUDEDIR}")

#find_package(Boost 1.68 REQUIRED COMPONENTS thread date_time)
#if(Boost_FOUND)
#	include_directories(${Boost_INCLUDE_DIRS})
#endif()

include_directories("${PROJECT_BINARY_DIR}")
include_directories("${INC_DIR}")

#add_subdirectory(../googletest ../googletest/lib)
#add_subdirectory(../test ../test)

file(GLOB_RECURSE INC_LIST ${INC_DIR}/*.h)

set(SRC_LIST 
    ${SRC_DIR}/ByteRingBuffer.cpp 
	${SRC_DIR}/TextHelper_win.cpp 
)

add_library(utils STATIC ${SRC_LIST} ${INC_LIST})

source_group("Inc" FILES ${INC_LIST})
source_group("Src" FILES ${SRC_LIST})

install (TARGETS utils DESTINATION bin)
install (FILES ${INC_DIR}/ByteRingBuffer.h 
    ${INC_DIR}/CriticalSection.h 
    ${INC_DIR}/ObjRingBuffer_tpl.h 
    DESTINATION include)