#CMakeList to make media_filter lib

cmake_minimum_required(VERSION 2.6)
project(media_filter)
#The version number
set(media_filter_VERSION_MAJOR 1)
set(media_filter_VERSION_MINOR 0)

#configure a header file to pass some of the CMake settings
#to the source code
configure_file(
    "${PROJECT_SOURCE_DIR}/media_filter_cfg.h.in"
    "${PROJECT_BINARY_DIR}/media_filter_cfg.h"
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
    ${SRC_DIR}/MediaData.cpp 
    ${SRC_DIR}/MemoryAllocator.cpp
    ${SRC_DIR}/PictureInfo.cpp
    ${SRC_DIR}/PcmData.cpp
    ${SRC_DIR}/IMediaFilter.cpp
    ${SRC_DIR}/BasicMediaFilter.cpp
)

add_library(media_filter STATIC ${SRC_LIST} ${INC_LIST})

source_group("Inc" FILES ${INC_LIST})
source_group("Src" FILES ${SRC_LIST})

install (TARGETS media_filter DESTINATION bin)
install (FILES ${INC_DIR}/BoostInc.h 
    ${INC_DIR}/MediaData.h 
    ${INC_DIR}/MemoryAllocator.h 
    ${INC_DIR}/AudioData.h 
    ${INC_DIR}/BasicMediaFilter.h 
    ${INC_DIR}/CodecMediaData.h 
    ${INC_DIR}/forcc.h 
    ${INC_DIR}/IMediaFilter.h 
    ${INC_DIR}/mediadata_tpl.h 
    ${INC_DIR}/mediafilter.h 
    ${INC_DIR}/PcmData.h 
    ${INC_DIR}/PictureInfo.h 
    ${INC_DIR}/VideoData.h 
    ${INC_DIR}/IVideoProcessor.h 
    DESTINATION include)
