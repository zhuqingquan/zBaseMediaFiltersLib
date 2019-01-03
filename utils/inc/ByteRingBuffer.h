#ifndef _COMMON_RING_BUFFER_H
#define _COMMON_RING_BUFFER_H

#include "CriticalSection.h"
#include <memory.h>
#ifndef NULL
	#define NULL 0
#endif
/*************************************************************
Ringbuffer有覆盖以及不覆盖二种功能可选：
1)选择覆盖时，数据未及时读取时，继续写入，可能导致读写位置标识重合，
继续写入则覆盖原有未读取的数据(读写pos重合时，认为size为0)。
2)选择非覆盖时，不会因为写数据导致读写位置重合，写位置将要等于读位置时
，会重新分配一个新buffer，复制原有数据到新的buffer，在新的buffer里面
写入新的数据。
3)对于写入为固定长度N，读取为固定长度M这样的操作，可以设置buffer初始
size为M和N的最小公倍数的整数倍，就可以实现覆盖功能，这样不读的时候，
一直写入必定能使读写pos重合。
/*************************************************************/

/*
采样率为44K，16bit，双声道一秒未压缩音频数据大小：
44100 * 16 * 2 / 8;
作为声音共用环形缓冲使用时，暂时约定buffer初始大小：
N = 采样率 * 位数 * 声道数(8秒PCM数据长度，选择覆盖功能)
*/
class CRingBuffer
{
public:
	CRingBuffer(int sizeInit, bool bCover = true);
	~CRingBuffer(void);

	void write(const char* data, int len);
	int read(char* data, int len);
	void clear();
	int size();
private:
	const char* data();
	void realloc(int len);
private:
	char* m_data;
	int m_posW;
	int m_posR;
	int m_capacity;
	bool m_bCover;		//是否覆盖未读取数据，默认覆盖
	CCriticalLock m_csLock;
};

/*
CRingBuffer::CRingBuffer(int sizeInit, bool bCover)
{
	m_posR = 0;
	m_posW = 0;
	m_bCover = bCover;
	m_capacity = sizeInit;

	m_data = new char[m_capacity];
	memset(m_data, 0x0, m_capacity);
}

CRingBuffer::~CRingBuffer(void)
{
	delete m_data;
}

const char* CRingBuffer::data()
{
	return m_data;
}

int CRingBuffer::size()
{
	if (m_posR != m_posW)
	{
		if (m_posR < m_posW)
		{
			return m_posW - m_posR;
		}
		else
		{
			return m_posW + m_capacity - m_posR;
		}
	}

	return 0;
}

int CRingBuffer::read(char* data, int len)
{
	if (NULL == data)
	{
		return 0;
	}

	CAutoLock autoLock(&m_csLock);
	if (size() < len)
	{
		return 0;
	}

	int read1 = m_capacity - m_posR;
	if (read1 < len)
	{
		int read2 = len - read1;
		memcpy(data, m_data + m_posR, read1);
		memcpy(data + read1, m_data, read2);
		m_posR = read2;
	}
	else
	{
		memcpy(data, m_data + m_posR, len); 
		m_posR = (m_posR + len) % m_capacity;
	}

	return len;
}

void CRingBuffer::write(const char* data, int len)
{
	if (NULL == data)
	{
		return;
	}

	CAutoLock autoLock(&m_csLock);
	realloc(len);

	int write1 = m_capacity - m_posW;
	if(write1 < len)
	{
		int write2 = len - write1;
		memcpy(m_data + m_posW, data, write1);
		memcpy(m_data, data + write1, write2);
		m_posW = write2;
	}
	else
	{
		memcpy(m_data + m_posW, data, len);
		m_posW = (m_posW + len) % m_capacity;
	}
}

void CRingBuffer::realloc(int len)
{
	int _size = size();
	int reserve = m_capacity - _size;
	if (!m_bCover)
	{
		reserve -= 1;
	}

	if (reserve < len)
	{
		int newCap = m_capacity * 2;
		char* newData = new char[newCap];
		memset(newData, 0x0, newCap);

		//数据copy
		if (0 == _size)
		{
			//整个buffer还不够一次性写入
			m_posR = 0;
			m_posW = 0;
		}
		else
		{
			int read1 = m_capacity - m_posR;
			if(read1 < _size)
			{
				int read2 = _size - read1;
				memcpy(newData, m_data + m_posR, read1);
				memcpy(newData + read1, m_data, read2);
			}
			else
			{
				memcpy(newData, m_data + m_posR, _size);
			}

			m_posR = 0;
			m_posW = _size;
		}

		delete m_data;
		m_data = newData;
		m_capacity = newCap;
	}
}
*/

#endif 