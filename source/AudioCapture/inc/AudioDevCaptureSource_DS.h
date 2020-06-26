/**
*	@file		AudioDevCaptureSource_DS.h
*	@author		zhuqingquan
*	@brief		使用DirectSound实现音频输入设备数据采集
*	@created	2019/01/22
**/

#pragma once
#ifndef _AUDIO_DEV_CAPTURE_SOURCE_DS_H_
#define _AUDIO_DEV_CAPTURE_SOURCE_DS_H_

#include "AudioDevCaptureSource.h"
#include <string>

struct IDirectSoundCapture;
struct IDirectSoundCaptureBuffer;

namespace zMedia
{

class AUDIO_CAPTURE_EXPORT_IMPORT AudioDevCaptureSource_DS : public AudioDevCaptureSource
{
public:
	AudioDevCaptureSource_DS(const char* id = nullptr);
	virtual ~AudioDevCaptureSource_DS();

	virtual const char* id() const { return m_id.c_str(); }

	virtual std::vector<WAVEFORMATEX> getSurpportFormat(const AudioDevInfo* auDevInfo);
	virtual int start(const AudioDevInfo* auDevInfo, const WAVEFORMATEX& waveFmt);
	virtual void stop();

	virtual int read(const uint8_t* buffer, size_t count, int index);

//private:
//	bool isValidWaveFmt(const WAVEFORMATEX& waveFmt) const;
private:
	std::string m_id;
	IDirectSoundCapture* m_capture;
	IDirectSoundCaptureBuffer* m_buffer;

	WAVEFORMATEX m_fmt;
	DWORD m_lastReadOffset;
};

}
#endif//_AUDIO_DEV_CAPTURE_SOURCE_DS_H_