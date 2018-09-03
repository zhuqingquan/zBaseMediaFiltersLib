#CMakeList to make media_filter lib

cmake_minimum_required(VERSION 2.6)
project(zBaseMediaFilterLib)
#The version number
set(ZBASEMEDIAFILTERLIB_VERSION_MAJOR 1)
set(ZBASEMEDIAFILTERLIB_VERSION_MINOR 0)

#configure a header file to pass some of the CMake settings
#to the source code
configure_file(
    "${PROJECT_SOURCE_DIR}/base_media_filters_lib_cfg.h.in"
    "${PROJECT_BINARY_DIR}/base_media_filters_lib_cfg.h"
    )
	
include_directories("${PROJECT_BINARY_DIR}")

add_subdirectory(${PROJECT_SOURCE_DIR}/utils ${PROJECT_BINARY_DIR}/utils)
add_subdirectory(${PROJECT_SOURCE_DIR}/media_filter ${PROJECT_BINARY_DIR}/media_filter)
