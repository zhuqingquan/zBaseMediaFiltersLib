#CMakeList to make media_filter lib

cmake_minimum_required(VERSION 2.6)
project(AudioCapture)
#The version number
set(AudioCapture_VERSION_MAJOR 1)
set(AudioCapture_VERSION_MINOR 0)

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

include_directories("${INC_DIR}")
include_directories("../../utils/inc")

#add_subdirectory(../googletest ../googletest/lib)
#add_subdirectory(../test ../test)

file(GLOB_RECURSE INC_LIST ${INC_DIR}/*.h)

set(SRC_LIST 
    ${SRC_DIR}/AudioDevCaptureSrc_CAA.cpp
    ${SRC_DIR}/AudioDevCaptureSource_DS.cpp 
	${SRC_DIR}/AudioDevManager.cpp 
)

add_definitions(-DUNICODE -D_UNICODE -DAUDIO_CAPTURE_DLL_EXPORT)

add_library(AudioCapture SHARED ${SRC_LIST} ${INC_LIST})
target_link_libraries(AudioCapture utils)

add_executable(AudioCaptureTest ../AudioCaptureTest/main.cpp)
target_link_libraries(AudioCaptureTest AudioCapture)

source_group("Inc" FILES ${INC_LIST})
source_group("Src" FILES ${SRC_LIST})

install (TARGETS AudioCapture DESTINATION bin)
install (FILES ${INC_DIR}/AudioDev.h 
    ${INC_DIR}/AudioDevManager.h 
    ${INC_DIR}/AudioDevCaptureSource.h 
	${INC_DIR}/AudioDevCaptureSource_DS.h 
    ${INC_DIR}/AudioDevCaptureSrc_CAA.h
    DESTINATION include)