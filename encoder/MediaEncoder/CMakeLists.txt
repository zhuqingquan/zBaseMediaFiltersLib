#CMakeList to make Media_Encoder lib

cmake_minimum_required(VERSION 2.6)
project(MediaEncoder)
#The version number
set(MediaEncoder_VERSION_MAJOR 1)
set(MediaEncoder_VERSION_MINOR 0)

#configure a header file to pass some of the CMake settings
#to the source code
configure_file(
    "${PROJECT_SOURCE_DIR}/MediaEncoder_cfg.h.in"
    "${PROJECT_BINARY_DIR}/MediaEncoder_cfg.h"
    )

set(SRC_DIR ${PROJECT_SOURCE_DIR})
set(INC_DIR ${PROJECT_SOURCE_DIR})

set(BOOST_ROOT ${3RD_LIB_PATH}/boost_1_68_0)
set(BOOST_INCLUDEDIR ${3RD_LIB_PATH}/boost_1_68_0/include)
set(BOOST_LIBARARYDIR ${3RD_LIB_PATH}/boost_1_68_0/lib)
include_directories("${BOOST_INCLUDEDIR}")

#find_package(Boost 1.68 REQUIRED COMPONENTS thread date_time)
#if(Boost_FOUND)
#	include_directories(${Boost_INCLUDE_DIRS})
#endif()

include_directories("${PROJECT_BINARY_DIR}")
include_directories("${INC_DIR}")
include_directories("${3RD_LIB_PATH}/ffmpeg-3.3.1/include")
include_directories("${3RD_LIB_PATH}/x264/include")
include_directories("${PROJECT_SOURCE_DIR}/../../utils/inc")
include_directories("${PROJECT_SOURCE_DIR}/../../media_filter/inc")
include_directories("${PROJECT_SOURCE_DIR}/../obs-qsv11/libmfx/include/msdk/include")
include_directories("${PROJECT_SOURCE_DIR}/../obs-qsv11")
include_directories("${PROJECT_SOURCE_DIR}/nvenc")

add_definitions(-DMEDIA_ENCODER_EXPORTS)

if(8==${CMAKE_SIZEOF_VOID_P})
	set(TARGET_PLATFORM "x64")
else()
	set(TARGET_PLATFORM "x86")
endif()
	
link_directories("${3RD_LIB_PATH}/ffmpeg-3.3.1/lib/release")
link_directories("${3RD_LIB_PATH}/boost_1_68_0/lib")
link_directories("${3RD_LIB_PATH}/x264/lib/${TARGET_PLATFORM}")

file(GLOB_RECURSE INC_LIST ${INC_DIR}/*.h)
file(GLOB_RECURSE NVENC_LIST_SRC ${SRC_DIR}/nvenc/*.cpp)
file(GLOB_RECURSE NVENC_LIST_INC ${SRC_DIR}/nvenc/*.h)

set(SRC_LIST 
    ${SRC_DIR}/FFMPEGAudioEncoder.cpp 
    ${SRC_DIR}/libobssqv11.cpp
    ${SRC_DIR}/MediaEncoder.cpp
    ${SRC_DIR}/NVSDKVideoEncoder.cpp
    ${SRC_DIR}/QSVVideoEncoder.cpp 
	${SRC_DIR}/x264Encoder.cpp 
)

add_library(MediaEncoder SHARED ${SRC_LIST} ${INC_LIST} ${NVENC_LIST_INC} ${NVENC_LIST_SRC})

SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
SET (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
SET (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")

target_link_libraries(MediaEncoder avcodec avutil avformat swresample swscale utils dxgi d3d11 d3d10 d3d9 )

source_group("Inc" FILES ${INC_LIST})
source_group("Src" FILES ${SRC_LIST})
source_group("nvenc" FILES ${NVENC_LIST_INC} ${NVENC_LIST_SRC})

install (TARGETS MediaEncoder DESTINATION bin)
install (FILES ${INC_DIR}/BoostInc.h 
    ${INC_DIR}/MediaEncoder.h 
    DESTINATION include)
