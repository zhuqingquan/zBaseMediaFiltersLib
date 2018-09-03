#ifndef _MEDIA_FILTER_CODEC_MEDIA_DATA_H_
#define _MEDIA_FILTER_CODEC_MEDIA_DATA_H_

#include "MediaData.h"

namespace zMedia
{
    class PictureCodec
    {
    public:
        typedef PictureCodec SelfType;
        typedef boost::shared_ptr<PictureCodec> SPtr;

		PictureCodec() : m_forcc(0), m_subtype(0), m_nPts(0), m_nDts(0) {}

        PictureCodec(uint32_t forcc, size_t length, const MemoryAllocator& allocator = MemoryAllocator())
			: m_forcc(forcc), m_subtype(0) 
			, m_nPts(0), m_nDts(0)
        {
            allocData(length, allocator);
        }
        
        PictureCodec(uint32_t forcc, BYTE* data, size_t length)
			: m_forcc(forcc), m_subtype(0)
			, m_nPts(0), m_nDts(0)
        {
            attachData(data, length);
        }

        virtual ~PictureCodec(){
            free();
        }

        void setFORCC(uint32_t forcc) { m_forcc = forcc; }
        uint32_t getFORCC() const { return m_forcc; }

        void setSubtype(uint32_t subtype) { m_subtype = subtype; }
        uint32_t getSubtype() const { return m_subtype; }
        
		void setPts(int64_t nPts) { m_nPts = nPts; }
		int64_t getPts() const { return m_nPts; }

		void setDts(int64_t nDts) { m_nDts = nDts; }
		int64_t getDts() const { return m_nDts; }

        size_t allocData(size_t length, const MemoryAllocator& allocator = MemoryAllocator())
        {
            return m_buf.malloc(length, allocator);
        }

        bool attachData(BYTE* data, size_t length, const MemoryAllocator& allocator = MemoryAllocator())
        {
            return m_buf.attachData(data, length, allocator);
        }

        size_t free()
        {
            return m_buf.free();
        }

        BYTE* data() { return m_buf.data(); }
        const BYTE* data() const { return m_buf.data(); }
        const MediaBuffer& buffer() const { return m_buf; }
        size_t size() const { return m_buf.getPayloadSize(); }
    private:
        PictureCodec(const PictureCodec& robj);
        PictureCodec& operator=(const PictureCodec& robj);
    private:
        uint32_t m_forcc;
        uint32_t m_subtype;
        MediaBuffer m_buf;
		int64_t m_nPts;
		int64_t m_nDts;

    };//class PictureCodec

    class AudioCodec
    {
    public:
        typedef AudioCodec SelfType;
        typedef boost::shared_ptr<AudioCodec> SPtr;

        AudioCodec() : m_forcc(0), m_subtype(0), m_timestamp(0) {}

        AudioCodec(uint32_t forcc, size_t length, const MemoryAllocator& allocator = MemoryAllocator())
            : m_forcc(forcc), m_subtype(0), m_timestamp(0)
        {
            allocData(length, allocator);
        }
        
        AudioCodec(uint32_t forcc, BYTE* data, size_t length)
            : m_forcc(forcc), m_subtype(0), m_timestamp(0)
        {
            attachData(data, length);
        }

        virtual ~AudioCodec() {
            free();
        }

        void setTimestamp(uint32_t ts) { m_timestamp = ts; }
        uint32_t getTimestamp() const { return m_timestamp; }
        
        void setFORCC(uint32_t forcc) { m_forcc = forcc; }
        uint32_t getFORCC() const { return m_forcc; }

        void setSubtype(uint32_t subtype) { m_subtype = subtype; }
        uint32_t getSubtype() const { return m_subtype; }

        size_t allocData(size_t length, const MemoryAllocator& allocator = MemoryAllocator())
        {
            return m_buf.malloc(length, allocator);
        }

        bool attachData(BYTE* data, size_t length, const MemoryAllocator& allocator = MemoryAllocator())
        {
            return m_buf.attachData(data, length, allocator);
        }

        size_t free()
        {
            return m_buf.free();
        }

        BYTE* data() { return m_buf.data(); }
        const BYTE* data() const { return m_buf.data(); }
        const MediaBuffer& buffer() const { return m_buf; }
        size_t size() const { return m_buf.getPayloadSize(); }
    private:
        AudioCodec(const AudioCodec& robj);
        AudioCodec& operator=(const AudioCodec& robj);
    private:
        uint32_t m_forcc;
        uint32_t m_subtype;
        uint32_t m_timestamp;
        MediaBuffer m_buf;
    };//class AudioCodec
}//namespace zMedia

#endif //_MEDIA_FILTER_CODEC_MEDIA_DATA_H_
