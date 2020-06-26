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
#include <vector>

namespace zMedia
{

class AUDIO_CAPTURE_EXPORT_IMPORT AudioDevCaptureSource
{
public:
	AudioDevCaptureSource(const char* id)
	{
		memset(&m_fmt, 0, sizeof(m_fmt));
	}

	virtual ~AudioDevCaptureSource() = 0
	{
	}

	virtual const char* id() const = 0;

	/**
	 *	@name		getSurpportFormat
	 *	@brief		获取默认支持的音频数据格式
	 *	@param[in]	const AudioDevInfo* auDevInfo 音频设备信息
	 *	@return		std::vector<WAVEFORMATEX> 支持的音频数据格式列表
	 **/
	virtual std::vector<WAVEFORMATEX> getSurpportFormat(const AudioDevInfo* auDevInfo) = 0;

	/**
	 *	@name		getDevInfo
	 *	@brief		调用start成功之后，可使用这个接口获取到设备的信息
	 **/
	const AudioDevInfo* getDevInfo() const { return &m_audioDevInfo; }
	/**
	*	@name		getWaveFormat
	*	@brief		调用start成功之后，可使用这个接口获取打开设备的WAVEFORMATEX
	*				用户在调用read之前应该先调用这个接口获取音频数据的格式
	**/
	const WAVEFORMATEX& getWaveFormat() const { return m_fmt; }

	virtual int start(const AudioDevInfo* auDevInfo, const WAVEFORMATEX& waveFmt) = 0;
	virtual void stop() = 0;

	virtual int read(const uint8_t* buffer, size_t count, int index) = 0;

protected:
	AudioDevInfo m_audioDevInfo;
	WAVEFORMATEX m_fmt;	
};

}
#endif//_AUDIO_DEV_CAPTURE_SOURCE_H_