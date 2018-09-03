#include "PictureInfo.h"

using namespace zMedia;

PictureRaw::PictureRaw()
{
}

PictureRaw::~PictureRaw(void)
{
}

bool PictureRaw::allocData( const PICTURE_FORMAT& _format, const MemoryAllocator& allocator /*= MemoryAllocator()*/ )
{
    if(!_format.isValid())	return false;
    int picRealSize = GetPictureSize(_format);
    size_t totalMallocSize = picRealSize;//*/(size_t)m_maxPixels;
    if(totalMallocSize!=m_buf.malloc(totalMallocSize, allocator))
        return false;

    m_format = _format;
    return m_buf.data()!=NULL;			
}

bool PictureRaw::attachData( uint8_t* pData, size_t len, const PICTURE_FORMAT& _format, const MemoryAllocator& allocator/* = MemoryAllocator()*/)
{
    if(!_format.isValid())	return false;
    if(!m_buf.attachData(pData, len, allocator))
        return false;

    m_format = _format;
    return true;
}

void PictureRaw::freeData()
{
    m_buf.free();
    m_format = zMedia::PICTURE_FORMAT();
}

