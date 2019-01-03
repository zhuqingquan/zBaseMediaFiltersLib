#pragma once
#ifndef _MEDIA_NV_SDK_VIDEO_ENCODER_H_
#define _MEDIA_NV_SDK_VIDEO_ENCODER_H_

#include "MediaEncoder.h"

struct NVSDKVideoEncoderPrivate;

namespace Media
{
	/**
	 *	@name		NVSDKVideoEncoder
	 *	@brief		基于NVIDIA的video codec sdk实现视频的编码
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
		 *	@brief		从编码器获取sps、pps、sei等信息
		 *	@return		int	0--成功  其他--失败
		 **/
		int loadHeaders();

		/**
		 *	@name		loadSDK
		 *	@brief		动态加载NVIDIA的编码DLL
		 *	@return		bool true--成功  false--失败
		 **/
		bool loadSDK();

		int initEncodeObj();
		int deinitEncodeObj();
	private:
		NVSDKVideoEncoderPrivate* m_private;
	};
}

#endif //_MEDIA_NV_SDK_VIDEO_ENCODER_H_