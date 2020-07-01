#pragma once
#ifndef _Z_MEDIA_AUDIO_OUTPUTER_H_
#define _Z_MEDIA_AUDIO_OUTPUTER_H_

#include "AudioDev.h"
#include <inttypes.h>

namespace zMedia
{
class AUDIO_CAPTURE_EXPORT_IMPORT AudioOutputer
{
public:
	AudioOutputer()
	{
		memset(&m_fmt, 0, sizeof(m_fmt));
	}

	virtual ~AudioOutputer() = 0
	{

	}

	/**
	 *	@name		id
	 *	@brief		��ȡ��ǰ������Ƶ����豸��id
	 **/
	virtual const char* id() const = 0;

	/**
	 *	@name	start
	 *	@brief	�������򿪣���Ƶ����豸
	 *			�豸��֮��������������ţ�������Buffer���ᴴ����ֻ�е���stop֮��Ż��ͷ�
	 *			�豸��֮����Ҫ����getWaveFormat��ȡ������豸��֧�ֵ���Ƶ���ݸ�ʽ��Ȼ���ٵ���writeд���Ӧ��ʽ����Ƶ����
	 *	@param[in] const AudioDevInfo* auDevInfo  �豸����Ϣ
	 *	@return	0--�ɹ�  ����--ʧ��
	 **/
	virtual int start(const AudioDevInfo* auDevInfo) = 0;

	/**
	 *	@name	stop
	 *	@brief	ֹͣ��Ƶ����豸
	 *			���ͷ�start�����������Դ
	 **/
	virtual void stop() = 0;

	/**
	 *	@name	write
	 *	@brief	����Ƶ����д�벥���豸�Ļ���buffer
	 *			д��ʱ�����ټ�����ݵ���ʵ��Ƶ��ʽ�������߱���ȷ��д����������豸֧�ֵ���Ƶ���ݸ�ʽһֱ
	 *			
	 *	@param[in]	const uint8_t* buffer ��д�����Ƶ�����׵�ַ
	 *	@param[in]	size_t count д������ݵĳ���
	 *				������ȷ��ÿ��д������ݳ���Ϊһ����Ƶframe (frame=sizeof(sample)*channel) ���ֽ�����������������д��ʧ��
	 *	@return		������ʵд������ݵ��ֽڳ���
	 **/
	virtual int write(const uint8_t* buffer, size_t count) = 0;

	/**
	 *	@name		getDevInfo
	 *	@brief		����start�ɹ�֮�󣬿�ʹ������ӿڻ�ȡ���豸����Ϣ
	 **/
	const AudioDevInfo* getDevInfo() const { return &m_audioDevInfo; }
	/**
	*	@name		getWaveFormat
	*	@brief		����start�ɹ�֮�󣬿�ʹ������ӿڻ�ȡ���豸��֧�ֲ��ŵ���Ƶ���ݵ�WAVEFORMATEX
	*				�û��ڵ���read֮ǰӦ���ȵ�������ӿڻ�ȡ��Ƶ���ݵĸ�ʽ
	**/
	const WAVEFORMATEX& getWaveFormat() const { return m_fmt; }

	/**
	 *	@name		getBufferFrameSize
	 *	@brief		����start�ɹ�֮�󣬵��ô˽ӿڻ�ȡ�豸�����ݻ�����г��ȣ���λΪFrame��frame = sizeof(sample) * channel
	 *				���Ϊstart����startʧ�ܣ����ֵΪ0
	 **/
	uint32_t getBufferFrameSize() const { return m_bufferSize; }
protected:
	AudioDevInfo m_audioDevInfo;
	WAVEFORMATEX m_fmt;
	uint32_t m_bufferSize = 0;

private:
	AudioOutputer(const AudioOutputer&);
	AudioOutputer& operator=(const AudioOutputer&);
};
}

#endif//_Z_MEDIA_AUDIO_OUTPUTER_H_