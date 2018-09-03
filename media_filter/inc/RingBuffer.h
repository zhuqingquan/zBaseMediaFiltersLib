//#pragma once
#ifndef _RING_BUFFER_H
#define _RING_BUFFER_H

#include <list>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
//#include "BoostInc.h"

namespace zMedia
{

/**
 *	@name	RingBuffer
 *	@brief	ѭ�����У�֧�ֵ��������߶��������
 *			������ͨ��insert�ӿڽ����ݲ�����У�������ͨ��DataPoller��ȡ����������
 *
 *			���е���ƻ������¼������裺
 *			1��ֻ��һ�����������ߣ����������
 *			2��ÿ�������߶�ϣ�����õ����е����ݣ�������©�������е��κ�һ������
 *			3��ÿ�������ߵ������ٶ��ǻ����Եȵģ������ڿ�Խ�����������Ĳ���
 **/
template<typename T>
class RingBuffer
{
public:
	/**
	 *	@name	DataPoller
	 *	@brief	ѭ�����е�������ȡ
	 *	@usage	���ȵ���RingBuffer�����createDataPoller��ȡ��Ӧ��������ȡ����
	 *			if(poller.dataValid()) const T& val = *poller. //��ȡ��ǰ����
	 *			poller.next(); //λ�Ƶ���һ������λ��
	 *			����ʹ��ʱ����RingBuffer�����releaseDataPoller�ͷ���ȡ�����ͷź���ȡ������ȡ�������޶���
	 **/
	class DataPoller
	{
	private:
		struct _DataPoller_Inner
		{
			const RingBuffer<T>& bufList;
			int index;
			uint32_t id;

			_DataPoller_Inner(const RingBuffer<T>& buf, int _index)
				: bufList(buf), index(_index), id(/*(uint32_t)this*/0)//fix me use rand number
			{}
		};
	public:
        DataPoller() 
        {
            //default construct, m_ptr is nullptr
        }
		DataPoller(const DataPoller& robj)
			: m_ptr(robj.m_ptr)
		{

		}
		DataPoller& operator=(const DataPoller& robj)
		{
			if(&robj==this)
				return *this;
			m_ptr.reset();
			m_ptr = robj.m_ptr;
			return *this;
		}
		~DataPoller()
		{
			m_ptr.reset();
		}

		/**
		 *	@name			dataValid
		 *	@brief			��ǰָ���Ƿ���Ч��DataPoller����ָ���޷���ȡ���ݵ�λ�ã������ʱ����*poller��Ϊ�޶���
		 *					ֻ����dataValidΪtrueʱ�ſɵ���*poller��
		 *	@return			bool true--��Ч false--��Ч
		 **/
		bool dataValid() const
		{
			return m_ptr && m_ptr->index!=-1;
		}
		/**
		 *	@name			operator*
		 *	@brief			��ȡ��������
		 **/
		const T& operator*() const
		{
			return m_ptr->bufList.m_dataList[m_ptr->index];
		}
		/**
		 *	@name			next
		 *	@brief			ָ����һ����
		 *	@return			bool true--�ɹ� false--ʧ��
		 **/
		bool next()
		{
			if(NULL==m_ptr)
				return false;
			if(m_ptr->index!=m_ptr->bufList.m_tail)
			{
				m_ptr->index = (m_ptr->index+1) % m_ptr->bufList.m_chunkCount;
				return true;
			}
			return false;
		}

		bool operator==(const DataPoller& robj) const
		{
			_DataPoller_Inner* inner = m_ptr.get();
			_DataPoller_Inner* inner_robj = robj.m_ptr.get();
			if(inner_robj==inner && inner!=NULL)
			{
				return &inner->bufList==&inner_robj->bufList && inner_robj->id==inner->id;
			}
			return false;
		}
		bool operator!=(const DataPoller& robj) const
		{
			_DataPoller_Inner* inner = m_ptr.get();
			_DataPoller_Inner* inner_robj = robj.m_ptr.get();
			if(inner_robj==inner && inner!=NULL)
			{
				return !(&inner->bufList==&inner_robj->bufList && inner_robj->id==inner->id);
			}
			return true;
		}
	private:
		DataPoller(const RingBuffer<T>& bufList, int index)
			: m_ptr(new _DataPoller_Inner(bufList, index))
		{

		}

		friend class RingBuffer<T>;
		boost::shared_ptr<_DataPoller_Inner> m_ptr;
	};

	RingBuffer()
		: m_tail(-1)
		, m_chunkCount(0)
	{

	}

	~RingBuffer(void)
	{
		//CAutoLock lock(m_csLock);
		clear();
	}

	/**
	 *	@name			init
	 *	@brief			��ʼ��
	 *	@param[in]		size_t chunkCount ��ʼ���������ܸ���
	 **/
	void init(size_t chunkCount)
	{
		if(m_dataList.size()!=0)
			return;
        boost::mutex::scoped_lock lock(m_pollerMutex);
		m_dataList.assign(chunkCount, T());
		m_tail = -1;
		m_chunkCount = chunkCount;
	}
	/**
	 *	@name			clear
	 *	@brief			������С���������գ��������������ܸ������ֲ���
	 **/
	void clear()
	{
        boost::mutex::scoped_lock lock(m_pollerMutex);
		m_dataList.clear();
		m_pollers.clear();
		m_dataList.assign(m_chunkCount, T());
		m_tail = -1;
		//m_chunkCount = 0;
	}

	/**
	 *	@name			createDataPoller
	 *	@brief			����������ȡ�����û��踺���ڲ�ʹ����ȡ����ʱ����releaseDataPoller�����ͷ�
	 *					������ܵ�����Ҳ�޷�insert����
	 *	@return			DataPoller ������ȡ�߶���
	 **/
	DataPoller createDataPoller()
	{
		RingBuffer<T>::DataPoller ret(*this, -1);
		if(m_tail!=-1)	//��������
		{
			RingBuffer<T>::DataPoller minDP = getMinIndex();
			ret = RingBuffer<T>::DataPoller(*this, minDP.m_ptr->index==-1 ? 0 : minDP.m_ptr->index);
		}
        boost::mutex::scoped_lock lock(m_pollerMutex);
		m_pollers.push_back(ret);	
		return ret;
	}
	/**
	 *	@name			releaseDataPoller
	 *	@brief			�ͷ�������ȡ�߶���
	 *	@param[in]		DataPoller & poller ������ȡ�߶���
	 **/
	void releaseDataPoller(DataPoller& poller)
	{
        boost::mutex::scoped_lock lock(m_pollerMutex);
		typename std::vector<typename RingBuffer<T>::DataPoller>::iterator iter = m_pollers.begin();
		for (; iter!=m_pollers.end(); iter++)
		{
			if(*iter==poller)
			{
				m_pollers.erase(iter);
				break;
			}
		}
		poller.m_ptr.reset();
	}
	/**
	 *	@name			getDataPollerCount
	 *	@brief			��ȡ������ȡ�ߵ�����
	 *	@return			size_t ����
	 **/
	size_t getDataPollerCount() const { return m_pollers.size(); }

	/**
	 *	@name			empty
	 *	@brief			�����Ƿ�Ϊ��
	 *	@return			bool 
	 **/
	bool empty() const
	{
		DataPoller minDP = getMinIndex();
		return minDP.m_ptr->index == m_tail;
	}
	/**
	 *	@name			full
	 *	@brief			�����Ƿ�����
	 *	@return			bool 
	 **/
	bool full() const
	{
		if(m_chunkCount==0)	return false;
		DataPoller minDP = getMinIndex();
		return minDP.m_ptr->index == ((m_tail + 1) % (int)m_chunkCount); /* when m_chunkCount very big, the converter maybe overflow */ 
	}
	/**
	 *	@name			size
	 *	@brief			��ǰ�����ݸ���
	 *	@return			size_t 
	 **/
	size_t size() const
	{
		return size(getMinIndex());
	}

	/**
	 *	@name			size
	 *	@brief			��������ȡ��ָ���λ�õ�����ĩβ���������ݸ���������������ȡ����ָ�������
	 *	@param[in]		const DataPoller & poller ������ȡ��
	 *	@return			size_t ���ݸ���
	 **/
	size_t size(const DataPoller& poller) const
	{
		return disToListTail(poller);
	}

	/**
	 *	@name			getChunkCount
	 *	@brief			��ȡ���е��ܳ���
	 *	@return			size_t 
	 **/
	size_t getChunkCount() const { return m_chunkCount; }
	/**
	 *	@name			insert
	 *	@brief			�������ݵ�����ĩβ
	 *	@param[in]		T & chunk ����
	 *	@return			bool true--����ɹ�  false--����ʧ��
	 **/
	bool insert(const T& chunk)
	{
		if(m_chunkCount<=0 || full())
			return false;
		int tmp = (m_tail + 1) % m_chunkCount;
		m_dataList[tmp] = chunk;
		m_tail = tmp;
		return true;
	}
private:
	DataPoller getMinIndex() const
	{
		if(m_tail==-1)	return RingBuffer<T>::DataPoller(*this, -1);
		size_t maxDis = 0;
		RingBuffer<T>::DataPoller ret(*this, 0);
        boost::mutex::scoped_lock lock(m_pollerMutex);
		for (size_t i=0; i<m_pollers.size(); i++)
		{
			size_t dis = disToListTail(m_pollers[i]);
			if(dis > maxDis)
			{
				maxDis = dis;
				ret = m_pollers[i];
			}
		}
		return ret;
	}
	size_t disToListTail(const DataPoller& dp) const
	{
		int dp_index = dp.m_ptr->index == -1 ? 0 : dp.m_ptr->index;
		return m_tail==-1 ? 0 : (dp_index <= m_tail ? m_tail-dp_index+1 : m_chunkCount - (dp_index-m_tail) + 1);
	}
private:
	//int p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p15;
	int m_tail;
	typedef std::vector<T> AudioChunkList;
	//mutable CCriticalLock m_csLock;
    mutable boost::mutex m_pollerMutex;
 	AudioChunkList m_dataList;
 	size_t m_chunkCount;
	std::vector<DataPoller> m_pollers;
};
/*
template<typename T>
RingBuffer<T>::RingBuffer()
: m_chunkCount(0)
, m_tail(-1)
{

}

template<typename T>
RingBuffer<T>::~RingBuffer( void )
{
	CAutoLock lock(m_csLock);
	clear();
}

template<typename T>
void RingBuffer<T>::init(size_t chunkCount )
{
	if(m_dataList.size()!=0)
		return;
	CAutoLock lock(m_csLock);
	m_dataList.assign(chunkCount, T());
	m_tail = -1;
	m_chunkCount = chunkCount;
}

template<typename T>
void RingBuffer<T>::clear()
{
	CAutoLock lock(m_csLock);
	m_dataList.clear();
	m_pollers.clear();
	m_dataList.assign(m_chunkCount, T());
	m_tail = -1;
	//m_chunkCount = 0;
}

template<typename T>
bool RingBuffer<T>::empty() const
{
	DataPoller minDP = getMinIndex();
	return minDP.m_ptr->index == m_tail;
}

template<typename T>
bool RingBuffer<T>::full() const 
{
	if(m_chunkCount==0)	return false;
	DataPoller minDP = getMinIndex();
	return minDP.m_ptr->index == ((m_tail + 1) % m_chunkCount);
}

template<typename T>
size_t RingBuffer<T>::size() const
{
	return size(getMinIndex());
}

template<typename T>
size_t RingBuffer<T>::size( const typename RingBuffer<T>::DataPoller& poller ) const
{
	return disToListTail(poller);
}

template<typename T>
bool RingBuffer<T>::insert( T& chunk )
{
	if(m_chunkCount<=0 || full())
		return false;
	int tmp = (m_tail + 1) % m_chunkCount;
	m_dataList[tmp] = chunk;
	m_tail = tmp;
	return true;
}

template<typename T>
typename RingBuffer<T>::DataPoller RingBuffer<T>::createDataPoller()
{
	RingBuffer<T>::DataPoller ret(*this, -1);
	if(m_tail!=-1)	//��������
	{
		RingBuffer<T>::DataPoller minDP = getMinIndex();
		ret = RingBuffer<T>::DataPoller(*this, minDP.m_ptr->index==-1 ? 0 : minDP.m_ptr->index);
	}
	CAutoLock lock(m_csLock);
	m_pollers.push_back(ret);	
	return ret;
}

template<typename T>
void RingBuffer<T>::releaseDataPoller( typename RingBuffer<T>::DataPoller& poller )
{
	CAutoLock lock(m_csLock);
	std::vector<typename RingBuffer<T>::DataPoller>::iterator iter = m_pollers.begin();
	for (; iter!=m_pollers.end(); iter++)
	{
		if(*iter==poller)
		{
			m_pollers.erase(iter);
			break;
		}
	}
	poller.m_ptr.reset();
}

template<typename T>
typename RingBuffer<T>::DataPoller RingBuffer<T>::getMinIndex() const
{
	if(m_tail==-1)	return RingBuffer<T>::DataPoller(*this, -1);
	size_t maxDis = 0;
	RingBuffer<T>::DataPoller ret(*this, 0);
	CAutoLock lock(m_csLock);
	for (size_t i=0; i<m_pollers.size(); i++)
	{
		size_t dis = disToListTail(m_pollers[i]);
		if(dis > maxDis)
		{
			maxDis = dis;
			ret = m_pollers[i];
		}
	}
	return ret;
}

template<typename T>
size_t RingBuffer<T>::disToListTail( const typename RingBuffer<T>::DataPoller& dp ) const
{
	int dp_index = dp.m_ptr->index == -1 ? 0 : dp.m_ptr->index;
	return m_tail==-1 ? 0 : (dp_index <= m_tail ? m_tail-dp_index+1 : m_chunkCount - (dp_index-m_tail) + 1);
}

template<typename T>
RingBuffer<T>::DataPoller::DataPoller(const typename RingBuffer<T>& bufList, int index)
: m_ptr(new RingBuffer<T>::DataPoller::_DataPoller_Inner(bufList, index))
{

}

template<typename T>
RingBuffer<T>::DataPoller::DataPoller( const typename RingBuffer<T>::DataPoller& robj )
: m_ptr(robj.m_ptr)
{

}

template<typename T>
typename RingBuffer<T>::DataPoller& RingBuffer<T>::DataPoller::operator=( const typename RingBuffer<T>::DataPoller& robj )
{
	if(&robj==this)
		return *this;
	m_ptr.reset();
	m_ptr = robj.m_ptr;
	return *this;
}

template<typename T>
RingBuffer<T>::DataPoller::~DataPoller()
{
	m_ptr.reset();
}

template<typename T>
const T& RingBuffer<T>::DataPoller::operator*() const
{
	return m_ptr->bufList.m_dataList[m_ptr->index];
}

template<typename T>
bool RingBuffer<T>::DataPoller::next()
{
	if(NULL==m_ptr)
		return false;
	if(m_ptr->index!=m_ptr->bufList.m_tail)
	{
		m_ptr->index = (m_ptr->index+1) % m_ptr->bufList.m_chunkCount;
		return true;
	}
	return false;
}

template<typename T>
bool RingBuffer<T>::DataPoller::dataValid() const
{
	return m_ptr && m_ptr->index!=-1;
}

template<typename T>
bool RingBuffer<T>::DataPoller::operator==(const typename RingBuffer<T>::DataPoller& robj) const
{
	_DataPoller_Inner* inner = m_ptr.get();
	_DataPoller_Inner* inner_robj = robj.m_ptr.get();
	if(inner_robj==inner && inner!=NULL)
	{
		return &inner->bufList==&inner_robj->bufList && inner_robj->id==inner->id;
	}
	return false;
}

template<typename T>
bool RingBuffer<T>::DataPoller::operator!=(const typename RingBuffer<T>::DataPoller& robj) const
{
	_DataPoller_Inner* inner = m_ptr.get();
	_DataPoller_Inner* inner_robj = robj.m_ptr.get();
	if(inner_robj==inner && inner!=NULL)
	{
		return !(&inner->bufList==&inner_robj->bufList && inner_robj->id==inner->id);
	}
	return true;
}

template<typename T>
RingBuffer<T>::DataPoller::_DataPoller_Inner::_DataPoller_Inner(const RingBuffer<T>& buf, int _index)
: bufList(buf), index(_index), id(0)//(uint32_t)thisfix me use rand number
{}
*/
}//namespace MediaFilter

#endif//#define _RING_BUFFER_H
