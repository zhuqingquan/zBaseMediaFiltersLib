#CMakeList to make media_filter lib

cmake_minimum_required(VERSION 2.6)
project(utils)
	
include_directories("inc")

set(INC_LIST 
	inc/Thread.h
	inc/QcCriticalLock.h
)

set(SRC_LIST 
	src/Thread.cpp
)

add_library(utils STATIC ${SRC_LIST} ${INC_LIST})

source_group("Inc" FILES ${INC_LIST})
source_group("Src" FILES ${SRC_LIST})