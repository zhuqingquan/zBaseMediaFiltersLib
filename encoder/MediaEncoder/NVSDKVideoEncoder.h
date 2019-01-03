#pragma once
#ifndef _MEDIA_NV_SDK_VIDEO_ENCODER_H_
#define _MEDIA_NV_SDK_VIDEO_ENCODER_H_

#include "MediaEncoder.h"

struct NVSDKVideoEncoderPrivate;

namespace Media
{
	/**
	 *	@name		NVSDKVideoEncoder
	 *	@brief		����NVIDIA��video codec sdkʵ����Ƶ�ı���
	 **/
	class NVSDKVideoEncoder : public VideoEncoder
	{
	public:
		NVSDKVideoEncoder();
		virtual ~NVSDKVideoEncoder();

		virtual bool init();
		virtual bool setConfig(const VideoEncoderConfig& vCfg);

		virtual bool encodeFrame(zMedia::VideoData::SPtr vframe);
		virtual bool getEncFrame(VideoOutputFrame& outFrame);

		virtual bool getValidPixelFormat(int* pixfmt, int& count);
	private:
		/**
		 *	@name		loadHeaders
		 *	@brief		�ӱ�������ȡsps��pps��sei����Ϣ
		 *	@return		int	0--�ɹ�  ����--ʧ��
		 **/
		int loadHeaders();

		/**
		 *	@name		loadSDK
		 *	@brief		��̬����NVIDIA�ı���DLL
		 *	@return		bool true--�ɹ�  false--ʧ��
		 **/
		bool loadSDK();

		int initEncodeObj();
		int deinitEncodeObj();
	private:
		NVSDKVideoEncoderPrivate* m_private;
	};
}

#endif //_MEDIA_NV_SDK_VIDEO_ENCODER_H_