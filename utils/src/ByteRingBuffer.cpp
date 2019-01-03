#include "ByteRingBuffer.h"
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

void CRingBuffer::clear()
{
	m_posW = m_posR = 0;
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

	CAutoLock autoLock(m_csLock);
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

	CAutoLock autoLock(m_csLock);
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