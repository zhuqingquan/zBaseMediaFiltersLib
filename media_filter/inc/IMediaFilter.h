/*
 * IMediaFilter.h
 *
 *  Created on: 2017年4月7日
 *      Author: zhuqingquan
 */

#ifndef IMEDIAFILTER_H_
#define IMEDIAFILTER_H_

#include "BoostInc.h"
#include "VideoData.h"
#include "AudioData.h"
#include <vector>
#include <string>
#include <map>

namespace zMedia
{

class IDataPuller;
class IDataPusher;
class IDataSink;

class IMediaFilter 
{
public:
	typedef IMediaFilter SelfType;
	typedef boost::shared_ptr<IMediaFilter> SPtr;
    typedef std::wstring IDType;

	IMediaFilter();
	virtual ~IMediaFilter() = 0;

    virtual const IDType& getID() const = 0;

	virtual int setDataPusher(const boost::shared_ptr<IDataPusher>& p);
	virtual boost::shared_ptr<IDataPusher> getDataPusher() const;
	virtual int setDataPuller(const boost::shared_ptr<IDataPuller>& p);
	virtual boost::shared_ptr<IDataPuller> getDataPuller() const;

    virtual boost::shared_ptr<IDataSink> createPullerSink() = 0;
    virtual bool releasePullerSink(boost::shared_ptr<IDataSink>& pullerSink) = 0;

    virtual bool add_v(VideoData::SPtr& vdata) = 0;
    virtual bool add_a(AudioData::SPtr& adata) = 0;
protected:
	virtual int pushData(VideoData::SPtr& spr);
	virtual int pushData(AudioData::SPtr& spr);

	boost::shared_ptr<IDataPusher> m_Pusher;
	boost::shared_ptr<IDataPuller> m_Puller;
};

class IDataPusher 
{
public:
	typedef IDataPusher SelfType;
	typedef boost::shared_ptr<IDataPusher> SPtr;

	IDataPusher();
	virtual ~IDataPusher() = 0;

	virtual int addFilter(const IMediaFilter::SPtr& in);
	virtual int remFilter(const IMediaFilter::SPtr& in);
	virtual int clear();
	virtual int getFilter(std::vector<IMediaFilter::SPtr>& listFilter);

	virtual int push(VideoData::SPtr& pic) = 0;
	virtual int push(AudioData::SPtr& pic) = 0;
protected:
	boost::mutex mutexFilters;
	std::vector<boost::shared_ptr<IMediaFilter> >vecFilters;
};

class IDataSink
{
public:
	typedef IDataSink SelfType;
	typedef boost::shared_ptr<IDataSink> SPtr;

    virtual ~IDataSink() = 0;
    virtual VideoData::SPtr get_v() = 0;
    virtual AudioData::SPtr get_a() = 0;
    virtual bool next_v() = 0;
    virtual bool next_a() = 0;
};

class IDataPuller 
{
public:
	typedef IDataPuller SelfType;
	typedef boost::shared_ptr<IDataPuller> SPtr;
    typedef std::map<IMediaFilter::IDType, VideoData::SPtr> DataCollectionV;
    typedef std::map<IMediaFilter::IDType, AudioData::SPtr> DataCollectionA;

	IDataPuller();
	virtual ~IDataPuller() = 0;

	virtual int addSink(const IDataSink::SPtr& in);
	virtual int remSink(const IDataSink::SPtr& in);
	virtual int clear();
	virtual int getFilter(std::vector<IDataSink::SPtr>& listSinks);

	virtual int pull(DataCollectionV& vDatas, UINT timeoutMillsec) = 0;
	virtual int pull(DataCollectionA& aDatas, UINT timeoutMillsec) = 0;

protected:
	boost::mutex m_mutexSinks;
	std::vector<IDataSink::SPtr> m_vecSinks;
};

} /* namespace zMedia */

#endif /* IMEDIAFILTER_H_ */
