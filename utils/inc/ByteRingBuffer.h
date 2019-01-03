#ifndef _COMMON_RING_BUFFER_H
#define _COMMON_RING_BUFFER_H

#include "CriticalSection.h"
#include <memory.h>
#ifndef NULL
	#define NULL 0
#endif
/*************************************************************
Ringbuffer�и����Լ������Ƕ��ֹ��ܿ�ѡ��
1)ѡ�񸲸�ʱ������δ��ʱ��ȡʱ������д�룬���ܵ��¶�дλ�ñ�ʶ�غϣ�
����д���򸲸�ԭ��δ��ȡ������(��дpos�غ�ʱ����ΪsizeΪ0)��
2)ѡ��Ǹ���ʱ��������Ϊд���ݵ��¶�дλ���غϣ�дλ�ý�Ҫ���ڶ�λ��ʱ
�������·���һ����buffer������ԭ�����ݵ��µ�buffer�����µ�buffer����
д���µ����ݡ�
3)����д��Ϊ�̶�����N����ȡΪ�̶�����M�����Ĳ�������������buffer��ʼ
sizeΪM��N����С�����������������Ϳ���ʵ�ָ��ǹ��ܣ�����������ʱ��
һֱд��ض���ʹ��дpos�غϡ�
/*************************************************************/

/*
������Ϊ44K��16bit��˫����һ��δѹ����Ƶ���ݴ�С��
44100 * 16 * 2 / 8;
��Ϊ�������û��λ���ʹ��ʱ����ʱԼ��buffer��ʼ��С��
N = ������ * λ�� * ������(8��PCM���ݳ��ȣ�ѡ�񸲸ǹ���)
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
	bool m_bCover;		//�Ƿ񸲸�δ��ȡ���ݣ�Ĭ�ϸ���
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

		//����copy
		if (0 == _size)
		{
			//����buffer������һ����д��
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