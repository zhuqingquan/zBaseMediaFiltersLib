#ifndef _COMMON_MEDIA_DATA_TPL_H_
#define _COMMON_MEDIA_DATA_TPL_H_

#include <boost/shared_ptr.hpp>

namespace zMedia
{
    template<class CodecData, class RawData>
    class mediadata_tpl //: public Ptr<mediadata_tpl<CodecData, RawData>>
    {
    public:
        typedef mediadata_tpl<CodecData, RawData> SelfType;
        typedef boost::shared_ptr<SelfType> SPtr;

        mediadata_tpl() {};
        mediadata_tpl(const typename CodecData::SPtr& codecData)
            : m_codecData(codecData)
        {
        }
        
        mediadata_tpl(const typename RawData::SPtr& rawData)
            : m_rawData(rawData)
        {
        }

		typename CodecData::SPtr getCodecData() const { return m_codecData; }
        void setCodecData(const typename CodecData::SPtr& codecData) { m_codecData = codecData; }

        typename RawData::SPtr getRawData() const { return m_rawData; }
        void setRawData(const typename RawData::SPtr& rawData) { m_rawData = rawData; }
    private:
        typename CodecData::SPtr m_codecData;
        typename RawData::SPtr m_rawData;
    };//class mediadata_tpl
}//namespace zMedia

#endif //_COMMON_MEDIA_DATA_TPL_H_
