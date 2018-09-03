#ifndef _Z_MEDIA_BASIC_MEDIA_FILTER_H_
#define _Z_MEDIA_BASIC_MEDIA_FILTER_H_

#include "IMediaFilter.h"

namespace zMedia
{
    /**
     * @name    SyncDataPusher
     * @brief   A kind of IDataPusher
     *          The data is pushed to the next filters sync
     **/
    class SyncDataPusher : public IDataPusher
    {
    public:
        SyncDataPusher(const IMediaFilter::SPtr& mf);
        virtual ~SyncDataPusher();

        virtual int push(VideoData::SPtr& pic);
        virtual int push(AudioData::SPtr& pic);

    private:
        const IMediaFilter::SPtr m_mf;
    };

    /**
     * @name    BasicMediaFilter
     * @brief   All kind of MediaFilter that have charaters below can inheritate this class:
     *          1.Not cache any data in the filter
     *          2.When media data is added, this filter pushed it imediatly
     *          3.When media data is adding, do little simple works in this filter.
     *          4.Not support pull mode in this filter.
     **/
    class BasicMediaFilter : public IMediaFilter
    {
    public:
        BasicMediaFilter();
        BasicMediaFilter(const IMediaFilter::IDType& id);
        virtual ~BasicMediaFilter();
        
        virtual const IMediaFilter::IDType& getID() const;
        virtual boost::shared_ptr<IDataSink> createPullerSink();
        virtual bool releasePullerSink(boost::shared_ptr<IDataSink>& pullerSink);

        virtual bool add_v(VideoData::SPtr& vdata);
        virtual bool add_a(AudioData::SPtr& adata);
    protected:
        /**
         * @name    onAddV
         * @brief   When the add_v() is called, the func will call.
         *          Filter can do some work for VideoData in this func
         **/
        virtual void onAddV(VideoData::SPtr& vdata);
        /**
         * @name    onAddA
         * @brief   When the add_a() is called, the func will call.
         *          Filter can do some work for AudioData in this func
         **/
        virtual void onAddA(AudioData::SPtr& adata);

        /**
         * @name    onPushV
         * @brief   Before push VideoData to the next filter, this func will call.
         **/
        virtual void onPushV(VideoData::SPtr& vdata);
        /**
         * @name    onPushA
         * @brief   Before push AudioData to the next filter, this func will call.
         **/
        virtual void onPushA(AudioData::SPtr& adata);
    private:
        const IMediaFilter::IDType m_id;
    };
}//namespace zMedia

#endif //_Z_MEDIA_BASIC_MEDIA_FILTER_H_
