/**
*	@file		AudioDevCaptureSource.h
*	@author		zhuqingquan
*	@brief		定义音频输入设备PCM数据采集的源接口
*	@created	2019/01/22
**/

#pragma once
#ifndef _AUDIO_DEV_CAPTURE_SOURCE_H_
#define _AUDIO_DEV_CAPTURE_SOURCE_H_

#include "AudioDev.h"
#include <inttypes.h>

namespace zMedia
{

class AUDIO_CAPTURE_EXPORT_IMPORT AudioDevCaptureSource
{
public:
	AudioDevCaptureSource(const char* id)
	{
	}

	virtual ~AudioDevCaptureSource() = 0
	{
	}

	virtual const char* id() const = 0;

	virtual int start(const AudioCaptureDevInfo* auDevInfo, const WAVEFORMATEX& waveFmt) = 0;
	virtual void stop() = 0;

	virtual int read(const uint8_t* buffer, size_t count, int index) = 0;
};

}
#endif//_AUDIO_DEV_CAPTURE_SOURCE_H_