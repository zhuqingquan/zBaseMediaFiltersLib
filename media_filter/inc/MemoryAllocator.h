/**
 *	@date		2015:12:3   17:07
 *	@name	 	MemoryAllocator.h
 *	@author		zhuqingquan	
 *	@brief		负责内存申请与释放的对象
 **/

//#pragma once
#ifndef _MEDIA_FILTER_MEMORY_ALLOCATOR_H_
#define _MEDIA_FILTER_MEMORY_ALLOCATOR_H_

#include <stdlib.h>
/*
#ifndef __cdecl
#define __cdecl __attribute__((__cdecl__))
#endif
*/
namespace zMedia
{
	typedef void* (*malloc_func)(size_t size);
	typedef void (*free_func)(void* memory);

	class MemoryAllocator
	{
	public:
		MemoryAllocator() : m_MallocFunc(NULL), m_FreeFunc(NULL){}

		MemoryAllocator(malloc_func m_func, free_func f_func)
			: m_MallocFunc(m_func), m_FreeFunc(f_func)
		{
		}

		malloc_func malloc_function() const { return m_MallocFunc; }
		free_func free_function() const { return m_FreeFunc; }

        static const MemoryAllocator std_allocator;
        
        friend bool operator==(const MemoryAllocator& lobj, const MemoryAllocator& robj);
	private:
		malloc_func m_MallocFunc;
		free_func m_FreeFunc;
	};

    bool operator==(const MemoryAllocator& lobj, const MemoryAllocator& robj);
}//namespace zMedia


#endif //_MEDIA_FILTER_MEMORY_ALLOCATOR_H_
