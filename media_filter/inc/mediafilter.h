//#pragma once
#ifndef _MEDIA_FILTER_H_
#define _MEDIA_FILTER_H_

#include <limits.h>
#include <inttypes.h>
#include "MemoryAllocator.h"

typedef uint8_t BYTE;
typedef uint32_t UINT;
#ifndef _WIN32
typedef char TCHAR; 
typedef bool BOOL;
#define FALSE 0
#define TRUE 1
typedef uint32_t DWORD;
#endif

namespace zMedia
{
	template <typename T>
	static inline T EnsureRange(const T nVal, const T nMin, const T nMax)
	{
		T tResult;
		tResult = nVal;
		if (tResult < nMin)
		{		
			tResult = nMin;
		}

		if (tResult > nMax)
		{
			tResult = nMax;
		}
		return tResult;
	}

	//对齐16字节，保证SSE指令正常
	inline int Align16Bytes(int value) { return (value <= INT_MAX-16) ? ((value + 15) & ~15) : value; }
	inline uint32_t Align16Bytes(uint32_t value) { return (value < UINT_MAX-16) ? ((value + 15) & ~15) : value; }
	//对齐32字节
	inline int Align32Bytes(int value) {  return (value <= INT_MAX-32) ? ((value + 31) & ~31) : value; }
	inline size_t Align32Bytes(size_t value) { return (value < UINT_MAX-32) ? ((value + 31) & ~31) : value; }
}//namespace zMedia

#endif //_MEDIA_FILTER_H_
