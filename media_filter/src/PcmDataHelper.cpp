#include "PcmDataHelper.h"

using namespace zMedia;

	inline bool MachineIsBigEndian()
	{
		return false;
	}

	inline bool MachineIsLittleEndian()
	{
		return !MachineIsBigEndian();
	}

	inline void SwapOrder(void* p, unsigned nBytes)
	{
		BYTE* ptr = (BYTE*)p;
		BYTE t;
		unsigned n;
		for(n = 0; n < nBytes>>1; n++)
		{
			t = ptr[n];
			ptr[n] = ptr[nBytes - n - 1];
			ptr[nBytes - n - 1] = t;
		}
	}


	template<class T, bool b_swap, bool b_signed, bool b_pad> class sucks_v2 
	{
	public:
		inline static void DoFixedpointConvert(const void* source, unsigned bps, unsigned count, AudioSample* buffer)
		{
			const char * src = (const char *) source;
			unsigned bytes = bps>>3;
			unsigned n;
			T max = ((T)1)<<(bps-1);

			T negmask = - max;

			double div = 1.0 / (double)(1<<(bps-1));
			for(n = 0; n < count; n++)
			{
				T temp;
				if (b_pad)
				{
					temp = 0;
					memcpy(&temp, src, bytes);
				}
				else
				{
					temp = *reinterpret_cast<const T*>(src);
				}

				if (b_swap) 
					SwapOrder(&temp,bytes);

				if (!b_signed) temp ^= max;

				if (b_pad)
				{
					if (temp & max) temp |= negmask;
				}

				if (b_pad)
					src += bytes;
				else
					src += sizeof(T);

				buffer[n] = (AudioSample)((double)temp * div);
			}
		}
	};

	template <class T,bool b_pad> class sucks 
	{ 
	public:
		inline static void DoFixedpointConvert(bool b_swap, bool b_signed, const void* source, unsigned bps,
			unsigned count, AudioSample* buffer)
		{
			if (sizeof(T) == 1)
			{
				if (b_signed)
				{
					sucks_v2<T, false, true, b_pad>::DoFixedpointConvert(source, bps, count, buffer);
				}
				else
				{
					sucks_v2<T, false, false, b_pad>::DoFixedpointConvert(source, bps, count, buffer);
				}
			}
			else if (b_swap)
			{
				if (b_signed)
				{
					sucks_v2<T, true, true, b_pad>::DoFixedpointConvert(source, bps, count, buffer);
				}
				else
				{
					sucks_v2<T, true, false, b_pad>::DoFixedpointConvert(source, bps, count, buffer);
				}
			}
			else
			{
				if (b_signed)
				{
					sucks_v2<T, false, true, b_pad>::DoFixedpointConvert(source,bps,count,buffer);
				}
				else
				{
					sucks_v2<T, false, false, b_pad>::DoFixedpointConvert(source,bps,count,buffer);
				}
			}
		}
	};



	int zMedia::short2float( const short* shortData, int shortCount, float* floatBuffer, int floatCount )
	{
		int nBits = 16;
		bool bSigned = nBits > 8;
		bool bNeedSwap = false;

		assert(floatCount >= shortCount);

		sucks<short, false>::DoFixedpointConvert(bNeedSwap,
			bSigned, shortData, nBits, shortCount, floatBuffer);

		return shortCount;
	}


	int zMedia::float2short(const float* floatData, int floatCount, short* dstShortBuffer, int shortBufferCount)
	{
		assert(shortBufferCount >= floatCount);

		const float* pSrc = floatData;
		short* pDst = dstShortBuffer;
		UINT processCount = floatCount;

		union {float f; DWORD i;} u;

		for (UINT i = 0; i < processCount; i++)
		{
			u.f = float(*pSrc + 384.0);
			if (u.i > 0x43c07fff) 
			{
				*pDst = 32767;
			}
			else if (u.i < 0x43bf8000) 
			{
				*pDst = -32768;
			}
			else 
			{
				*pDst = short(u.i - 0x43c00000);
			}
			pSrc++;
			pDst++;
		}

		return floatCount;
	}

	PcmData::SPtr zMedia::convertPcmData( const PcmData::SPtr& srcData, UINT dstChannels, UINT dstSampleRate, AudioSampelTypeSize dstSampleTypeSize )
	{
		if(srcData->empty())
			return PcmData::SPtr();
		//所有属性相同，不需转换
		if(srcData->sampleSize()==dstSampleTypeSize	&& dstChannels==srcData->channels() && dstSampleRate==srcData->sampleRate())
			return srcData;
		
		//只是处理short to float，其他的转换之后再添加
		if(srcData->sampleSize()==AudioSampleSize_SHORT && dstSampleTypeSize==AudioSampleSize_FLOAT
			&& dstChannels==srcData->channels() && dstSampleRate==srcData->sampleRate())
		{
			PcmData::SPtr  dst(new PcmData(dstChannels, dstSampleRate, dstSampleTypeSize, srcData->getTimeCount()));
			assert(dst);
			
			UINT dstCapacitySampleCoutn = dst->capacity() / dstSampleTypeSize;
			zMedia::short2float((short*)srcData->data(), srcData->sampleCount(), (float*)dst->data(), dstCapacitySampleCoutn);
			dst->setWritePos(dst->capacity());
			return dst;
		}
		//其他情况暂时返回失败
		return PcmData::SPtr();
	}

	PcmData::SPtr zMedia::clonePcmData( const PcmData::SPtr& srcData )
	{
		PcmData::SPtr clonedPcmData(new PcmData);
		clonedPcmData->allocBuffer(srcData->capacity(), srcData->memAllocator());
		clonedPcmData->setChannels(srcData->channels());
		clonedPcmData->setSampleRate(srcData->sampleRate());
		clonedPcmData->setSampleSize(srcData->sampleSize());
		clonedPcmData->setVolume(srcData->volume());
		clonedPcmData->setIsMuted(srcData->isMuted());
		clonedPcmData->setTimeStamp(srcData->getTimeStamp());
		clonedPcmData->setData(srcData->data(), srcData->size());//copy the pcm data
		return clonedPcmData;
	}

	PcmDataConvert::PcmDataConvert()
	//:m_buffer(NULL)
	:m_dataSize(0)
	//, m_bufferSize(0)
	{

	}

	PcmDataConvert::~PcmDataConvert()
	{
		m_buf.free();
		m_dataSize = 0;
// 		if (m_buffer)
// 		{
// 			free(m_buffer);
// 			m_buffer = NULL;
// 			m_dataSize = 0;
// 		}
	}

	bool PcmDataConvert::float2short( const BYTE* floatData, int bytesCount )
	{
		alloc(bytesCount/2);

		int floatSampleCount = bytesCount / sizeof(float);
		int shortSampleCount = m_buf.length() / sizeof(short);

		int samples = zMedia::float2short((const float*)floatData, floatSampleCount, (short*)m_buf.data(), shortSampleCount);
		m_dataSize = samples * sizeof(short);

		return samples == shortSampleCount;
	}

	bool PcmDataConvert::short2float( const BYTE* shortData, int bytesCount )
	{
		clear();

		alloc(bytesCount * 2);

		int shortSampleCount = bytesCount / sizeof(short);
		int floatSampleCount = m_buf.length() / sizeof(float);

		int samples = zMedia::short2float((const short*)shortData, shortSampleCount, (float*)m_buf.data(), floatSampleCount);

		m_dataSize = samples * sizeof(float);

		return samples == shortSampleCount;
	}

	void PcmDataConvert::alloc( int size )
	{
        assert(size>0);
		assert(m_dataSize == 0);
		if(m_buf.length()<(size_t)size)
		{
			m_buf.malloc(size);
		}

// 		if (m_bufferSize < size)
// 		{
// 			if (m_buffer)
// 			{
// 				free(m_buffer);
// 			}
// 			m_buffer = (BYTE*)malloc(size);
// 			m_bufferSize = size;
// 		}
	}
