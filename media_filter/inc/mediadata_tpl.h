#ifndef _COMMON_MEDIA_DATA_TPL_H_
#define _COMMON_MEDIA_DATA_TPL_H_

#include <boost/shared_ptr.hpp>
#include <map>

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

		/**
		*	@name		setExtraData
		*	@brief		将type类型的附加信息关联到当前mediadata数据上
		*	@param[in]	int type 附加数据的类型
		*	@param[in]	void * extraData 附加数据的指针
		*	@return		bool true--成功  false--失败
		**/
		bool setExtraData(int type, void* extraData)
		{
			m_extraDatas[type] = extraData;
			return true;
		}

		/**
		*	@name		getExtraData
		*	@brief		获取关联的type类型的附加信息
		*	@param[in]	int type 附加数据的类型
		*	@return		void* 附加数据指针
		**/
		void* getExtraData(int type)
		{
			std::map<int, void*>::const_iterator iter = m_extraDatas.find(type);
			if (iter == m_extraDatas.end())
				return NULL;
			return iter->second;
		}
    private:
        typename CodecData::SPtr m_codecData;
        typename RawData::SPtr m_rawData;
		std::map<int, void*> m_extraDatas;
    };//class mediadata_tpl
}//namespace zMedia

#endif //_COMMON_MEDIA_DATA_TPL_H_
