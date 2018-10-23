#include "MediaData.h"
#include "mediafilter.h"

using namespace zMedia;

MediaBuffer::MediaBuffer()
    : m_data(NULL), m_bufLen(0)
    , m_type(MEDIA_BUFFER_TYPE_UNKNOW)
    , m_isNeedFree(false)
    , m_payloadOffset(0), m_payloadSize(0)
	, m_glTextureID(0), m_glTexInternalFmt(0), m_glTexTarget(0)
{

}

MediaBuffer::MediaBuffer(size_t length, const MemoryAllocator& allocator /*= MemoryAllocator()*/)
    : m_data(NULL), m_bufLen(0)
    , m_type(MEDIA_BUFFER_TYPE_UNKNOW)
    , m_isNeedFree(false)
    , m_payloadOffset(0), m_payloadSize(0)
{
    malloc(length, allocator);
}

MediaBuffer::MediaBuffer(BYTE* pData, size_t len, const MemoryAllocator& allocator /*= MemoryAllocator()*/)
    : m_data(NULL), m_bufLen(0)
    , m_type(MEDIA_BUFFER_TYPE_UNKNOW)
    , m_isNeedFree(false)
    , m_payloadOffset(0), m_payloadSize(0)
{
    attachData(pData, len, allocator);
}

MediaBuffer::~MediaBuffer()
{
    free();
}

/*virtual*/ size_t MediaBuffer::malloc(size_t length, const MemoryAllocator& allocator/* = MemoryAllocator()*/)
{
    //malloc just do malloc, if there is already malloced or attachdata before, malloc failed.
	if(m_data || m_bufLen>0 || m_type != MEDIA_BUFFER_TYPE_UNKNOW)
	{
        return 0;
	}
	if(length<=0)
	{
		return 0;
	}
	size_t realLen = Align32Bytes(length);
	malloc_func mf = allocator.malloc_function();
    zMedia::MemoryAllocator allocatorTmp = allocator;
    if(NULL==mf)
    {
        allocatorTmp = zMedia::MemoryAllocator::std_allocator;
        mf = allocatorTmp.malloc_function();
    }
	BYTE* pdata = (BYTE*)mf(realLen);
	assert(pdata);
	if(!pdata)	return 0;
	m_data = pdata;
	m_bufLen = length;
    m_allocator = allocatorTmp;
	m_type = MEDIA_BUFFER_TYPE_MEMORY;
	return m_bufLen;
}

/*virtual*/ size_t MediaBuffer::free()
{
    //call free_func to free the data when need.
    //reset all var in Mediabuffer
	if (m_type == MEDIA_BUFFER_TYPE_OPENGL_TEX)
	{
		m_glTexInternalFmt = 0;
		m_glTexTarget = 0;
		m_glTextureID = 0;
		m_type = MEDIA_BUFFER_TYPE_UNKNOW;
		return 0;
	}
	if(m_data==0 || m_bufLen<=0)
		return 0;
    free_func ff = m_allocator.free_function();
    if(ff)
    {
        ff(m_data);
    }
    size_t len = m_bufLen;
	m_data = NULL;
	m_bufLen = 0;
    m_payloadOffset = 0;
    m_payloadSize = 0;
	m_isNeedFree = false;
	m_allocator = MemoryAllocator();
	m_type = MEDIA_BUFFER_TYPE_UNKNOW;
	return len;
}

/*virtual*/bool MediaBuffer::attachData(BYTE* pData, size_t len, const MemoryAllocator& allocator/* = MemoryAllocator()*/)
{
    //attachdata just do attach, if there is already malloced or attachdata before, attach failed.
	if(pData==NULL || len<=0)
		return false;
    if(m_data || m_bufLen>0 || m_type != MEDIA_BUFFER_TYPE_UNKNOW)
        return false;
	m_data = pData;
	m_bufLen = len;
    m_payloadOffset = 0;
    m_payloadSize = 0;
	m_allocator = allocator;
	m_type = MEDIA_BUFFER_TYPE_MEMORY;
	return true;
}

bool MediaBuffer::attachData(uint32_t glTextureID, uint32_t glTexTarget, uint32_t glTexInternalFmt)
{
	if (m_data != NULL || m_bufLen > 0 || m_type != MEDIA_BUFFER_TYPE_UNKNOW)
		return false;
	if (0 == glTextureID || 0 == glTexTarget || 0 == glTexInternalFmt)
		return false;
	m_glTextureID = glTextureID;
	m_glTexTarget = glTexTarget;
	m_glTexInternalFmt = glTexInternalFmt;
	m_type = MEDIA_BUFFER_TYPE_OPENGL_TEX;
	return true;
}
