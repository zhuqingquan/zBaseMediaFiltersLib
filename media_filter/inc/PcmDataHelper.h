#ifndef _MEDIA_FILTER_COMMON_PCM_DATA_HELPER_H_
#define _MEDIA_FILTER_COMMON_PCM_DATA_HELPER_H_

#include "PcmData.h"

namespace zMedia
{
	// xuyupeng 测试内部约定
#define AudioSample float 
#define Default_ReadTime       20     // 20ms 读取一次
#define Default_SampleRate     44100  //HZ
#define Default_Channels       2      //2声道
#define Default_BitsPerSample	16
#define Default_AudioFrameBytes 7056	//双声道：float每帧大小:882*2*sizeof(float)	short每帧大小:882*2*sizeof(short)
#define Default_Volume			50


	//定义PcmData类对象的智能指针类型
	//typedef COMMON_API boost::shared_ptr<PcmData> PcmDataPtr;

	PcmData::SPtr clonePcmData(const PcmData::SPtr& srcData);

	PcmData::SPtr convertPcmData(const PcmData::SPtr& srcData, uint32_t dstChannels, uint32_t dstSampleRate, AudioSampelTypeSize dstSampleTypeSize);

	int float2short(const float* floatData, int floatCount, short* shortBuffer, int shortCount);
	int short2float(const short* shortData, int shortCount, float* floatBuffer, int floatCount);

	class PcmDataConvert
	{
	public:
		PcmDataConvert();
		~PcmDataConvert();

		bool float2short(const BYTE* floatData, int bytesCount );
		bool short2float(const BYTE* shortData, int bytesCount );

		const BYTE* data() const { return m_buf.data(); }
		size_t size() const { return m_dataSize; }
		void clear() { m_dataSize = 0; }

	private:
		void alloc(int size);

	private:
		MediaBuffer m_buf;
		//BYTE* m_buffer;
		int m_dataSize;
		//int m_bufferSize;
	};
}//namespace zMedia

#endif //_MEDIA_FILTER_COMMON_PCM_DATA_HELPER_H_
