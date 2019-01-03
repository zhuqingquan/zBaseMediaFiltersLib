/** 
 *	@file		QSVVideoEncoder.h
 *	@author		zhuqingquan
 *	@brief		Implementation of Intel QSV video Encoder
 *	@created	2018/06/03
 **/
#pragma once
#ifndef _MEDIA_ENCODER_QSV_VIDEO_ENCODER_H_
#define _MEDIA_ENCODER_QSV_VIDEO_ENCODER_H_

#include "MediaEncoder.h"

namespace Media
{
	struct QSVVideoEncoderPrivate;

	class QSVVideoEncoder : public Media::VideoEncoder
	{
	public:
		QSVVideoEncoder();
		virtual ~QSVVideoEncoder();

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
	private:
		Media::QSVVideoEncoderPrivate* m_private;
	};
}

#endif //_MEDIA_ENCODER_QSV_VIDEO_ENCODER_H_