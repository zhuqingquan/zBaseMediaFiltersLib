#pragma once
#ifndef _MEDIA_DW_HW_VIDEO_ENCODER_H_
#define _MEDIA_DW_HW_VIDEO_ENCODER_H_

#include <tchar.h>
#include "MediaEncoder.h"

extern "C"
{
	bool loadLibDll(TCHAR* dllPathName);
	bool freeLibDll();
};

class DwVideoEncoder;

namespace Media
{
	class DwHWVideoEncoder : public VideoEncoder
	{
	public:
		DwHWVideoEncoder(Media::VIDEO_CODEC_IMPL codecType);
		virtual ~DwHWVideoEncoder();

		virtual bool init();
		virtual bool setConfig(const VideoEncoderConfig& vCfg);

		virtual bool encodeFrame(boost::shared_ptr<VideoFrame> vframe);
		virtual bool getEncFrame(VideoOutputFrame& outFrame);

		virtual bool getValidPixelFormat(int* pixfmt, int& count);

	private:
		DwVideoEncoder* m_encoderImpl;
		VideoEncoderConfig m_encConfig;
	};
}

#endif //_MEDIA_DW_HW_VIDEO_ENCODER_H_