#include "BasicMediaFilter.h"

using namespace zMedia;

SyncDataPusher::SyncDataPusher(const IMediaFilter::SPtr& mf)
: m_mf(mf)
{
}

SyncDataPusher::~SyncDataPusher()
{
}


int SyncDataPusher::push(VideoData::SPtr& pic)
{
    boost::mutex::scoped_lock lock(mutexFilters);
    std::vector<IMediaFilter::SPtr>::const_iterator iter = vecFilters.begin();
    for(; iter!=vecFilters.end(); iter++)
    {
        IMediaFilter::SPtr pmf = *iter;
        if(pmf) pmf->add_v(pic);
    }
    return 0;
}

int SyncDataPusher::push(AudioData::SPtr& pic)
{
    boost::mutex::scoped_lock lock(mutexFilters);
    std::vector<IMediaFilter::SPtr>::const_iterator iter = vecFilters.begin();
    for(; iter!=vecFilters.end(); iter++)
    {
        IMediaFilter::SPtr pmf = *iter;
        if(pmf) pmf->add_a(pic);
    }
    return 0;
}

BasicMediaFilter::BasicMediaFilter()
{
}

BasicMediaFilter::BasicMediaFilter(const IMediaFilter::IDType& id)
: m_id(id)
{
}

BasicMediaFilter::~BasicMediaFilter()
{
}

const IMediaFilter::IDType& BasicMediaFilter::getID() const
{
    return m_id;
}

boost::shared_ptr<IDataSink> BasicMediaFilter::createPullerSink()
{
    return IDataSink::SPtr();
}

bool BasicMediaFilter::releasePullerSink(boost::shared_ptr<IDataSink>& pullerSink)
{
    return true;
}


bool BasicMediaFilter::add_v(VideoData::SPtr& vdata)
{
    onAddV(vdata);
    onPushV(vdata);
    pushData(vdata);
    return true;
}

bool BasicMediaFilter::add_a(AudioData::SPtr& adata)
{
    onAddA(adata);
    onPushA(adata);
    pushData(adata);
    return true;
}

void BasicMediaFilter::onAddV(VideoData::SPtr& vdata)
{
}

void BasicMediaFilter::onAddA(AudioData::SPtr& adata)
{
}

void BasicMediaFilter::onPushV(VideoData::SPtr& vdata)
{
}

void BasicMediaFilter::onPushA(AudioData::SPtr& adata)
{
}
