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
 *	@brief	循环队列，支持单个生产者多个消费者
 *			生产者通过insert接口将数据插入队列，消费者通过DataPoller拉取队列中数据
 *
 *			队列的设计基于以下几个假设：
 *			1、只有一个数据生产者，多个消费者
 *			2、每个消费者都希望能拿到所有的数据，即不会漏掉队列中的任何一个数据
 *			3、每个消费者的消费速度是基本对等的，不存在跨越两个数量级的差异
 **/
template<typename T>
class RingBuffer
{
public:
	/**
	 *	@name	DataPoller
	 *	@brief	循环队列的数据拉取
	 *	@usage	首先调用RingBuffer对象的createDataPoller获取对应的数据拉取对象。
	 *			if(poller.dataValid()) const T& val = *poller. //获取当前数据
	 *			poller.next(); //位移到下一个数据位置
	 *			不再使用时调用RingBuffer对象的releaseDataPoller释放拉取对象。释放后拉取对象拉取的数据无定义
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
		 *	@brief			当前指针是否有效，DataPoller可能指向无法获取数据的位置，如果此时调用*poller行为无定义
		 *					只有在dataValid为true时才可调用*poller。
		 *	@return			bool true--有效 false--无效
		 **/
		bool dataValid() const
		{
			return m_ptr && m_ptr->index!=-1;
		}
		/**
		 *	@name			operator*
		 *	@brief			获取数据内容
		 **/
		const T& operator*() const
		{
			return m_ptr->bufList.m_dataList[m_ptr->index];
		}
		/**
		 *	@name			next
		 *	@brief			指向下一数据
		 *	@return			bool true--成功 false--失败
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
	 *	@brief			初始化
	 *	@param[in]		size_t chunkCount 初始化队列中总个数
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
	 *	@brief			清除队列。将数据清空，但队列中数据总个数保持不变
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
	 *	@brief			创建数据拉取对象。用户需负责在不使用拉取对象时调用releaseDataPoller进行释放
	 *					否则可能导致再也无法insert数据
	 *	@return			DataPoller 数据拉取者对象
	 **/
	DataPoller createDataPoller()
	{
		RingBuffer<T>::DataPoller ret(*this, -1);
		if(m_tail!=-1)	//已有数据
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
	 *	@brief			释放数据拉取者对象
	 *	@param[in]		DataPoller & poller 数据拉取者对象
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
	 *	@brief			获取数据拉取者的总数
	 *	@return			size_t 总数
	 **/
	size_t getDataPollerCount() const { return m_pollers.size(); }

	/**
	 *	@name			empty
	 *	@brief			队列是否为空
	 *	@return			bool 
	 **/
	bool empty() const
	{
		DataPoller minDP = getMinIndex();
		return minDP.m_ptr->index == m_tail;
	}
	/**
	 *	@name			full
	 *	@brief			队列是否已满
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
	 *	@brief			当前的数据个数
	 *	@return			size_t 
	 **/
	size_t size() const
	{
		return size(getMinIndex());
	}

	/**
	 *	@name			size
	 *	@brief			从数据拉取者指向的位置到队列末尾包含的数据个数，包括数据拉取者所指向的数据
	 *	@param[in]		const DataPoller & poller 数据拉取者
	 *	@return			size_t 数据个数
	 **/
	size_t size(const DataPoller& poller) const
	{
		return disToListTail(poller);
	}

	/**
	 *	@name			getChunkCount
	 *	@brief			获取队列的总长度
	 *	@return			size_t 
	 **/
	size_t getChunkCount() const { return m_chunkCount; }
	/**
	 *	@name			insert
	 *	@brief			插入数据到队列末尾
	 *	@param[in]		T & chunk 数据
	 *	@return			bool true--插入成功  false--插入失败
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
	if(m_tail!=-1)	//已有数据
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
