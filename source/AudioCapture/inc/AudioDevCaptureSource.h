/**
*	@file		AudioDevCaptureSource.h
*	@author		zhuqingquan
*	@brief		������Ƶ�����豸PCM���ݲɼ���Դ�ӿ�
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
	 *	@brief		��ȡĬ��֧�ֵ���Ƶ���ݸ�ʽ
	 *	@param[in]	const AudioDevInfo* auDevInfo ��Ƶ�豸��Ϣ
	 *	@return		std::vector<WAVEFORMATEX> ֧�ֵ���Ƶ���ݸ�ʽ�б�
	 **/
	virtual std::vector<WAVEFORMATEX> getSurpportFormat(const AudioDevInfo* auDevInfo) = 0;

	/**
	 *	@name		getDevInfo
	 *	@brief		����start�ɹ�֮�󣬿�ʹ������ӿڻ�ȡ���豸����Ϣ
	 **/
	const AudioDevInfo* getDevInfo() const { return &m_audioDevInfo; }
	/**
	*	@name		getWaveFormat
	*	@brief		����start�ɹ�֮�󣬿�ʹ������ӿڻ�ȡ���豸��WAVEFORMATEX
	*				�û��ڵ���read֮ǰӦ���ȵ�������ӿڻ�ȡ��Ƶ���ݵĸ�ʽ
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