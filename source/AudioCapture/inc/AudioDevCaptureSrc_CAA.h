/**
*	@file		AudioDevCaptureSrc_CAA.h
*	@author		zhuqingquan
*	@brief		使用Windows Core Audio API接口实现音频PCM数据采集功能
*				继承并实现接口AudioDevCaptureSource
*	@created	2019/02/20
**/
#pragma once
#ifndef _AUDIO_DEV_CAPTURE_SRC_CAA_H_
#define _AUDIO_DEV_CAPTURE_SRC_CAA_H_

#include "AudioDevCaptureSource.h"
#include <string>

struct IAudioClient;
struct IAudioCaptureClient;

namespace zMedia
{
	class AUDIO_CAPTURE_EXPORT_IMPORT AudioDevCaptureSrc_CAA : public AudioDevCaptureSource
	{
	public:
		AudioDevCaptureSrc_CAA(const char* id);
		virtual ~AudioDevCaptureSrc_CAA();

		virtual const char* id() const { return m_id.c_str(); }

		virtual std::vector<WAVEFORMATEX> getSurpportFormat(const AudioDevInfo* auDevInfo);
		virtual int start(const AudioDevInfo* auDevInfo, const WAVEFORMATEX& waveFmt);
		virtual void stop();

		virtual int read(const uint8_t* buffer, size_t count, int index);
	private:
		std::string m_id;
		IAudioClient* m_audioClient;
		IAudioCaptureClient* m_captureClient;
	};
}

#endif//_AUDIO_DEV_CAPTURE_SRC_CAA_H_