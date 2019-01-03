/** 
 *	@file		MediaEncoder.h
 *	@author		zhuqingquan
 *	@brief		Define the interface MediaEncoder, do video or audio data encoder.
 *	@created	2018/06/03
 **/
#pragma once
#ifndef _MEDIA_ENCODER_INTERFACE_H_
#define _MEDIA_ENCODER_INTERFACE_H_

#include <inttypes.h>
#include "boost/shared_ptr.hpp"
#include "AudioData.h"
#include "VideoData.h"

class VideoFrame;
class AudioFrame;

#ifdef MEDIA_ENCODER_EXPORTS
#define MEDIA_ENC_API __declspec(dllexport)
#else
#define MEDIA_ENC_API __declspec(dllimport)
#endif

namespace Media
{
	enum VIDEO_CODEC_IMPL
	{
		HW_CODEC_NVIDIA_NVENC = 1,
		HW_CODEC_INTEL_QUICKSYNC = 2,
		HW_CODEC_NVIDIA_NVENC_HEVC = 3,
		HW_CODEC_INTEL_QUICKSYNC_HEVC = 4,
		SW_CODEC_X264 = 100,
		SW_CODEC_X265 = 101,
		SW_CODEC_QYX265 = 102,
	};

	enum VIDEO_CODEC_ABILITY
	{
		ABILITY_NVIDIA_AVC    = 1 << 0,
		ABILITY_NVIDIA_HEVC   = 1 << 1,
		ABILITY_INTEL_AVC     = 1 << 2,
		ABILITY_INTEL_HEVC    = 1 << 3,
		ABILITY_SOFTWARE_AVC  = 1 << 4,
		ABILITY_SOFTWARE_HEVC = 1 << 5,
		ABILITY_SOFTWARE_QYHEVC = 1 << 6,
	};

	enum ENCODED_VIDEO_FRAME_TYPE
	{
		FRAME_TYPE_UNKNOW = 0,
		FRAME_TYPE_P = 1,
		FRAME_TYPE_I = 2,
		FRAME_TYPE_IDR = 3,
		FRAME_TYPE_B = 4,
	};

	struct VideoEncoderConfig
	{
		VideoEncoderConfig()
		{
			memset(this, 0, sizeof(*this));
		}

		// enum for rate_control type
		enum tagRcMode{
			RC_CQP = 0,
			RC_VBR,
			RC_CBR,
			RC_VBR_MINQP,
			RC_2PASS_IMG,
			RC_2PASS,
			RC_2PASS_VBR,
		};

		uint32_t width;
		uint32_t height;
		uint32_t fps;
		uint32_t gop; // the interval of IDR(key) frames
		uint32_t numBFrame; // max consecutive B frames
		uint32_t rcMode; // rate control mode (cbr recommended) 
		// 0=fixed QP, 1=VBR, 2=CBR, 3=VBR_MINQP, 
		// 4=Multi pass encoding optimized for image quality, 
		// 5= Multi pass encoding optimized for maintaining frame size,
		// 6=Multi pass VBR
		uint32_t avgBitrate; // average bitrate
		uint32_t maxBitrate; // maximum bitrate
		uint32_t minBitrate; // minimum bitrate
		uint32_t outputPkgMode; // 0 : annex b, 1 : mp4
		char configString[1024];// option strings to config more parameters.

		int  qp;
		int	 thread_count;
		int  pixfmt;			//enum value define in PictureInfo.h zMedia::E_PIXFMT, default is 0
		int  colorspace;		//enum value define in Picture.h Video::video_colorspace, default is 0
		int  range_type;		//enum value define in Picture.h Video::video_range_type, default is 0
	};

	struct VideoOutputFrame
	{
		VideoOutputFrame(unsigned char* pbuf, uint32_t bufLen)
		{
			memset(this, 0, sizeof(VideoOutputFrame));
			frameData = pbuf;
			frameSize = bufLen;
		}

		~VideoOutputFrame()
		{
			frameData = NULL;
			frameSize = 0;
		}

		uint64_t getTimestamp() const { return dts; }

		unsigned char* frameData; // data pointer
		uint32_t frameSize; // encoded length
		ENCODED_VIDEO_FRAME_TYPE frameType;
		uint32_t dts;
		uint32_t pts;
		int32_t nalRefIdc;
		int32_t nalType;
	};

	/**
	 *	@name		VideoEncoder
	 *	@brief		��Ƶ������
	 *				����ʵ�ֵ�ʲô��Ƶ��ʽ�ı����ɼ̳������
	 **/
	class MEDIA_ENC_API VideoEncoder
	{
	public:
		/**
		 *	@name		setConfig
		 *	@brief		���ñ�����������
		 *				��init֮ǰ��Ҫ����һ�θýӿڡ�
		 *				init�ɹ�֮���ٴε��øýӿ�ʱ���ܸı�VideoEncoderConfig�еĿ�ߵ���Ҫ���ԣ���������ʧ��
		 *				���磺init�ɹ�֮���ٵ��øýӿڿ���ʵ���޸ı�������
		 *	@param[in]	const VideoEncoderConfig & vCfg ����������
		 *	@return		bool true--�ɹ�  false--ʧ��
		 **/
		virtual bool setConfig(const VideoEncoderConfig& vCfg) = 0;

		/**
		 *	@name		init
		 *	@brief		��ʼ��������
		 *				��Ҫ�ȳɹ�����һ��setConfig���ڱ����������Խ������ò��ܽ��б������ĳ�ʼ��
		 *	@return		bool true--�ɹ� false--ʧ��
		 **/
		virtual bool init() = 0;

		/**
		 *	@name		encodeFrame
		 *	@brief		����һ֡��Ƶ֡
		 *				�ýӿ�ֻ����Ƶ֡�ύ��������������ȷ���ܹ���ñ�������Ƶ֡
		 *				�ýӿڿɽ���NULL���������NULL֮���ܹ��ӱ������л�ȡ��һ֡����������򷵻سɹ������򷵻�false
		 *				ÿ�ε���encodeFrame֮��ɵ���getEncFrame��ȡ����ɹ���֡
		 *	@param[in]	zMedia::VideoData::SPtr vframe ԭʼ��Ƶ֡��δ����
		 *	@return		bool true--�ɹ�  false--ʧ��
		 **/
		virtual bool encodeFrame(zMedia::VideoData::SPtr vframe) = 0;

		/**
		 *	@name		getEncFrame
		 *	@brief		��ȡ�ѳɹ��������Ƶ֡
		 *	@param[out]	VideoOutputFrame & outFrame �ɹ��������Ƶ֡
		 *	@return		bool true--�ɹ� false--ʧ��
		 *				�����سɹ�ʱoutFrame�������������ȷ�����ݣ�������ʧ����outFrame������δ���κ��޸�
		 **/
		virtual bool getEncFrame(VideoOutputFrame& outFrame) = 0;
	
		/**
		 *	@name		getValidPixelFormat
		 *	@brief		��ȡ������֧�ֵ����ظ�ʽ
		 *	@param[in,out]	int* pixfmt ����ͼƬ�����ظ�ʽ���飬�ɱ�������֧�ֵ����ظ�ʽֵ�����������������ظ�ʽ�ο�Video::E_COLORSPACE
		 *	@param[in,out]	int& count ��������ظ�ʽ������ܳ��ȣ����֧�ֵ����ظ�ʽ�ĸ���
		 *	@return		bool true--�ɹ�  false--ʧ�ܣ������������count̫Сʱ���ܵ���ʧ��
		 **/
		virtual bool getValidPixelFormat(int* pixfmt, int& count) = 0;

		virtual ~VideoEncoder()  = 0{};
	};

	enum AUDIO_ENC_CODEC_ID
	{
		AUDIO_CODEC_ID_UNKNOW = 0,
		AUDIO_CODEC_ID_AAC = 1,
		AUDIO_CODEC_ID_MP3,
	};

	enum AUDIO_ENC_PROFILE
	{
		AUDIO_ENC_PROFILE_AAC_HE = 0,
		AUDIO_ENC_PROFILE_AAC_LC = 1,
	};

	struct AudioEncoderConfig
	{
		AudioEncoderConfig()
		{
			memset(this, 0, sizeof(AudioEncoderConfig));
		}

		AUDIO_ENC_CODEC_ID codecID;	//�������ͣ�ʾ����mmioFOURCC('A', 'A', 'C', ' ')�� mmioFOURCC('M', 'P', '3', ' ')
		int nSampleRate;		//�����ʣ�ʾ����44100��48000
		int nBitrate;			//���ʣ�ʾ����128*1024
		int nChannels;			//������ʾ����2(˫�������Ƽ�)��1(������)
		AUDIO_ENC_PROFILE profile;	//����ȼ����鿴enum AUDIO_ENC_PROFILE
	};

	struct AudioOutputFrame
	{
		AudioOutputFrame(unsigned char* pbuf, uint32_t bufLen)
		{
			memset(this, 0, sizeof(AudioOutputFrame));
			frameData = pbuf;
			frameSize = bufLen;
		}

		AudioOutputFrame()
		{
			frameData = NULL;
			frameSize = 0;
		}

		uint64_t getTimestamp() const { return dts; }

		unsigned char* frameData; // data pointer
		uint32_t frameSize; // encoded length
		AUDIO_ENC_CODEC_ID codecID;
		uint64_t dts;
		uint64_t pts;
	};

	class MEDIA_ENC_API AudioEncoder
	{
	public:
		virtual bool init() = 0;
		virtual bool setConfig(const AudioEncoderConfig& aCfg) = 0;

		virtual bool encodeFrame(zMedia::AudioData::SPtr vframe) = 0;
		virtual bool getEncFrame(AudioOutputFrame& outFrame) = 0;
	
		virtual ~AudioEncoder() = 0 {};
	};
}


extern "C"
{
	MEDIA_ENC_API int GetVideoEncAbility();

	MEDIA_ENC_API int CreateVideoEncoder(Media::VIDEO_CODEC_IMPL codecType, Media::VideoEncoder** outEncoder);
	MEDIA_ENC_API int ReleaseVideoEncoder(Media::VideoEncoder** encoder);

	MEDIA_ENC_API int CreateAudioEncoder(Media::AUDIO_ENC_CODEC_ID codecID, Media::AudioEncoder** outEncoder);
	MEDIA_ENC_API int ReleaseAudioEncoder(Media::AudioEncoder** encoder);

}

#endif //_MEDIA_ENCODER_INTERFACE_H_