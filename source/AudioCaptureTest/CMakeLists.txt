#CMakeList to make media_filter lib

cmake_minimum_required(VERSION 2.6)
project(AudioCaptureTest)
#The version number
set(AudioCaptureTest_VERSION_MAJOR 1)
set(AudioCaptureTest_VERSION_MINOR 0)

set(SRC_DIR ${PROJECT_SOURCE_DIR})
set(INC_DIR ${PROJECT_SOURCE_DIR})

set(BOOST_ROOT ${PROJECT_SOURCE_DIR}/../3rd/boost_1_68_0)
set(BOOST_INCLUDEDIR ${PROJECT_SOURCE_DIR}/../3rd/boost_1_68_0/include)
set(BOOST_LIBARARYDIR ${PROJECT_SOURCE_DIR}/../3rd/boost_1_68_0/lib)
include_directories("${BOOST_INCLUDEDIR}")

#find_package(Boost 1.68 REQUIRED COMPONENTS thread date_time)
#if(Boost_FOUND)
#	include_directories(${Boost_INCLUDE_DIRS})
#endif()

include_directories("${INC_DIR}" ${PROJECT_SOURCE_DIR}/../AudioCapture/inc)

#add_subdirectory(../googletest ../googletest/lib)
#add_subdirectory(../test ../test)

file(GLOB_RECURSE INC_LIST ${INC_DIR}/*.h)

set(SRC_LIST 
    ${SRC_DIR}/main.cpp  
)

add_definitions(-DUNICODE -D_UNICODE)

add_executable(AudioCaptureTest SHARED ${SRC_LIST} ${INC_LIST})

source_group("Inc" FILES ${INC_LIST})
source_group("Src" FILES ${SRC_LIST})
