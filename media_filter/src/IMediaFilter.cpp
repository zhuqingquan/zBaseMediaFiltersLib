
#include "IMediaFilter.h"

using namespace zMedia;

// IDataPusher
IDataPusher::IDataPusher()
{
}

IDataPusher::~IDataPusher()
{
}

int IDataPusher::addFilter(const IMediaFilter::SPtr& in)
{
    boost::mutex::scoped_lock lock(mutexFilters);
    vecFilters.push_back(in);
    return 0;
}

int IDataPusher::remFilter(const IMediaFilter::SPtr& in)
{
    boost::mutex::scoped_lock lock(mutexFilters);
    vecFilters.erase( remove(vecFilters.begin(), vecFilters.end(), in), vecFilters.end());
    return 0;
}

int IDataPusher::clear()
{
    boost::mutex::scoped_lock lock(mutexFilters);
    vecFilters.clear();
    return 0;
}

int IDataPusher::getFilter(std::vector<IMediaFilter::SPtr >& listFilter)
{
    boost::mutex::scoped_lock lock(mutexFilters);
    listFilter = vecFilters;
    return 0;
}

// IDataPuller
IDataPuller::IDataPuller()
{
}

IDataPuller::~IDataPuller()
{
}

int IDataPuller::addSink(const IDataSink::SPtr& in)
{
    boost::mutex::scoped_lock lock(m_mutexSinks);
    m_vecSinks.push_back(in);
    return 0;
}

int IDataPuller::remSink(const IDataSink::SPtr& in)
{
    boost::mutex::scoped_lock lock(m_mutexSinks);
    m_vecSinks.erase( remove(m_vecSinks.begin(), m_vecSinks.end(), in), m_vecSinks.end());
    return 0;
}

int IDataPuller::clear()
{
    boost::mutex::scoped_lock lock(m_mutexSinks);
    m_vecSinks.clear();
    return 0;
}

int IDataPuller::getFilter(std::vector<IDataSink::SPtr>& listSinks)
{
    boost::mutex::scoped_lock lock(m_mutexSinks);
    listSinks = m_vecSinks;
    return 0;
}

// IMediaFilter
IMediaFilter::IMediaFilter()
{
}

IMediaFilter::~IMediaFilter()
{
}
/*
	int IMediaFilter::pullData(PictrueData::SPtr& spr){
		if (pPuller){
			pPuller->pull(spr);
			return 1;
		}
		return 0;
	}

	int IMediaFilter::pushData(const PictrueData::SPtr& spr){
		if (pPusher){
			pPusher->push(spr);
			return 1;
		}
		return 0;
	}

	int IMediaFilter::onPull(PictrueData::SPtr& spr){
		return 0;
	}

	int IMediaFilter::onPush(const PictrueData::SPtr& spr){
		return 0;
	}

	int IMediaFilter::pullData(PcmData::SPtr& spr){
		if (pPuller){
			pPuller->pull(spr);
			return 1;
		}
		return 0;
	}

	int IMediaFilter::pushData(const PcmData::SPtr& spr){
		if (pPusher){
			pPusher->push(spr);
			return 1;
		}
		return 0;
	}

	int IMediaFilter::onPull(PcmData::SPtr& spr){
		return 0;
	}
	int IMediaFilter::onPush(const PcmData::SPtr& spr){
		return 0;
	}
*/
int IMediaFilter::setDataPusher(const boost::shared_ptr<IDataPusher>& p)
{
    m_Pusher = p;
    return 0;
}

boost::shared_ptr<IDataPusher> IMediaFilter::getDataPusher() const
{
    return m_Pusher;
}

int IMediaFilter::setDataPuller(const IDataPuller::SPtr& p)
{
    m_Puller = p;
    return 0;
}

IDataPuller::SPtr IMediaFilter::getDataPuller() const
{
    return m_Puller;
}

int IMediaFilter::pushData(VideoData::SPtr& spr)
{
    if(m_Pusher)
    {
        return m_Pusher->push(spr);
    }
    return -1;
}

int IMediaFilter::pushData(AudioData::SPtr& spr)
{
    if(m_Pusher)
    {
        return m_Pusher->push(spr);
    }
    return -1;
}
/*
	int IMediaFilter::addPullFilter(const boost::shared_ptr<IMediaFilter>& in){
		if (pPuller){
			pPuller->addFilter(in);
			return 1;
		}
		return 0;
	}

	int IMediaFilter::remPullFilter(const boost::shared_ptr<IMediaFilter>& in){
		if (pPuller){
			pPuller->remFilter(in);
			return 1;
		}
		return 0;
	}

	int IMediaFilter::addPushFilter(const boost::shared_ptr<IMediaFilter>& in){
		if (pPusher){
			pPusher->addFilter(in);
			return 1;
		}
		return 0;
	}

	int IMediaFilter::remPushFilter(const boost::shared_ptr<IMediaFilter>& in){
		if (pPusher){
			pPusher->remFilter(in);
			return 1;
		}
		return 0;
	}
*/
