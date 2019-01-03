/** 
 *	@file		x264Encoder.h
 *	@author		zhuqingquan
 *	@brief		Implementation of x264 software H264 encoder
 *	@created	2018/06/12
 **/
#pragma once
#ifndef _MEDIA_ENCODER_X264_ENCODer_H_
#define _MEDIA_ENCODER_X264_ENCODer_H_

#include "MediaEncoder.h"

namespace Media
{
	struct x264EncoderPrivate;

	class x264Encoder : public Media::VideoEncoder
	{
	public:
		x264Encoder();
		virtual ~x264Encoder();

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

	private:
		Media::x264EncoderPrivate* m_private;
	};
}

#endif//_MEDIA_ENCODER_X264_ENCODer_H_