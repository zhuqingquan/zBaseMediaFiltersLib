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

    /**
     * @name    data
     * @brief   Get the beginning pointer of the buffer
     * @return  const BYTE* The beginning pointer of the buffer.
     **/
	const BYTE* data() const { return m_data; }
    /**
     * @name    data
     * @brief   Get the beginning pointer of the buffer
     * @return  const BYTE* The beginning pointer of the buffer.
     **/
	BYTE* data() { return m_data; }
    /**
     * @name    length
     * @brief   Get the total bytes malloced in this buffer.
     **/
	size_t length() const { return m_bufLen; }
	const MemoryAllocator& memAllocator() const { return m_allocator; }

	size_t malloc(size_t length, const MemoryAllocator& allocator = MemoryAllocator());
	size_t free();

	bool attachData(BYTE* pData, size_t len, const MemoryAllocator& allocator = MemoryAllocator());

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
	int32_t m_type;
	MemoryAllocator m_allocator;	//内存申请对象，默认使用共享内存的申请释放方法
	bool m_isNeedFree;
};

} /* namespace zMedia */

#endif /* IMEDIADATA_H_ */
