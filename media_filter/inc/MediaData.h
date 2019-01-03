/*
 * IMediaData.h
 *
 *  Created on: 2016年3月8日
 *      Author: Administrator
 */

#ifndef IMEDIADATA_H_
#define IMEDIADATA_H_

#include "BoostInc.h"
#include <inttypes.h>
#include "MemoryAllocator.h"

typedef uint8_t BYTE;

/**
* @name    MediaBufferType
* @brief   Identify the type of MediaBuffer object.
**/
typedef enum _MediaBufferType
{
	MEDIA_BUFFER_TYPE_UNKNOW = 0,
	MEDIA_BUFFER_TYPE_MEMORY,		// The buffer is allocated in normal system memory
	MEDIA_BUFFER_TYPE_OPENGL_TEX,	// The buffer is allocated in video card by opengl driver
	MEDIA_BUFFER_TYPE_D3D11_TEX,	// The buffer is allocated in video card by D3D11 driver
}MediaBufferType;

namespace zMedia {

/**
 * @name    MediaBuffer
 * @brief   Wrap of a memory buffer
 *          have many function to support manager the start point, write pos, payload of the data.
 **/
class MediaBuffer {
public:
	typedef MediaBuffer SelfType;
	typedef boost::shared_ptr<MediaBuffer> SPtr;

	MediaBuffer();

	MediaBuffer(size_t length, const MemoryAllocator& allocator = MemoryAllocator());

	MediaBuffer(BYTE* pData, size_t len, const MemoryAllocator& allocator = MemoryAllocator());

	~MediaBuffer();

	MediaBufferType type() const { return m_type; }
    /**
     * @name    data
     * @brief   Get the beginning pointer of the buffer
	 *			This func is only valid when type()==MEDIA_BUFFER_TYPE_MEMORY
	 * @return  const BYTE* The beginning pointer of the buffer.
     **/
	const BYTE* data() const { return m_data; }
    /**
     * @name    data
     * @brief   Get the beginning pointer of the buffer
	 *			This func is only valid when type()==MEDIA_BUFFER_TYPE_MEMORY
	 * @return  const BYTE* The beginning pointer of the buffer.
     **/
	BYTE* data() { return m_data; }
    /**
     * @name    length
     * @brief   Get the total bytes malloced in this buffer.
	 *			This func is only valid when type()==MEDIA_BUFFER_TYPE_MEMORY
     **/
	size_t length() const { return m_bufLen; }

	/**
	 * @name    memAllocator
	 * @brief   Get the memory allocator of this buffer object
	 **/
	const MemoryAllocator& memAllocator() const { return m_allocator; }

	/**
	 * @name    malloc
	 * @brief   User allocator to malloc length bytes memory in system memory
	 *			The type()==MEDIA_BUFFER_TYPE_MEMORY after call malloc success
	 * @param[in]	size_t length bytes count be allocated.
	 * @param]in]	const MemoryAllocator& allocator reference of a MemoryAllocator
	 * @return		size_t bytes count that allocated success. <=0--failed.
	 **/
	size_t malloc(size_t length, const MemoryAllocator& allocator = MemoryAllocator());

	/**
	 * @name    free
	 * @brief   free the memory be allocated and reset the state.
	 * @return	size_t bytes count that last time allocated or attach success. 
	**/
	size_t free();

	/**
	* @name			attachData
	* @brief		Attach a block system memory to this object
	*				The type()==MEDIA_BUFFER_TYPE_MEMORY after call attachData() success
	* @param[in]	BYTE* pData A block system memory
	* @param[in]	size_t len bytes count be allocated.
	* @param]in]	const MemoryAllocator& allocator reference of a MemoryAllocator
	* @return		bool true--success false--failed
	**/
	bool attachData(BYTE* pData, size_t len, const MemoryAllocator& allocator = MemoryAllocator());

	/**
	* @name			attachData
	* @brief		Attach a opengl texture to this object
	*				After call this func success, 
	*				The type()==MEDIA_BUFFER_TYPE_OPENGL_TEX, and data() return NULL.
	* @param[in]	uint32_t glTextureID opengl texture id.
	* @param[in]	uint32_t glTexTarget Opengl texture target
	* @param]in]	uint32_t glTexInternalFmt Opengl texture internal format
	* @return		bool true--success false--failed
	**/
	bool attachData(uint32_t glTextureID, uint32_t glTexTarget, uint32_t glTexInternalFmt);

	uint32_t glTextureID() const { return m_glTextureID; }
	uint32_t glTexTarget() const { return m_glTexTarget; }
	uint32_t glTexInternalFmt() const { return m_glTexInternalFmt; }

    size_t getPayloadOffset() const { return m_payloadOffset; }
    bool setPayloadOffset(size_t payloadOffset)
    {
        if(payloadOffset>=m_bufLen) return false;
        int payloadsize_inc = payloadOffset - m_payloadOffset;//may be negitive
        if(payloadsize_inc>0 && (size_t)payloadsize_inc>m_payloadSize)
            return false;
        m_payloadOffset = payloadOffset;
        m_payloadSize -= payloadsize_inc;
        return true;
    }

    size_t getPayloadSize() const { return m_payloadSize; }
    bool setPayloadSize(size_t payloadSize)
    {
        size_t resevered = m_bufLen - m_payloadOffset;
        if(payloadSize>resevered)
            return false;
        m_payloadSize = payloadSize;
        return true;
    }
protected:
	BYTE* m_data;
	size_t m_bufLen;
    size_t m_payloadOffset;
    size_t m_payloadSize;
	MediaBufferType m_type;
	MemoryAllocator m_allocator;	//内存申请对象，默认使用共享内存的申请释放方法
	bool m_isNeedFree;
	uint32_t m_glTextureID;
	uint32_t m_glTexTarget;
	uint32_t m_glTexInternalFmt;
};

} /* namespace zMedia */

#endif /* IMEDIADATA_H_ */
