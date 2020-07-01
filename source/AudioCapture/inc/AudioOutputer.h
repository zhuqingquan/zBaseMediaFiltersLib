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
	 *	@brief		获取当前对象音频输出设备的id
	 **/
	virtual const char* id() const = 0;

	/**
	 *	@name	start
	 *	@brief	启动（打开）音频输出设备
	 *			设备打开之后用于输出（播放）声音的Buffer将会创建，只有调用stop之后才会释放
	 *			设备打开之后需要调用getWaveFormat获取该输出设备所支持的音频数据格式，然后再调用write写入对应格式的音频数据
	 *	@param[in] const AudioDevInfo* auDevInfo  设备的信息
	 *	@return	0--成功  其他--失败
	 **/
	virtual int start(const AudioDevInfo* auDevInfo) = 0;

	/**
	 *	@name	stop
	 *	@brief	停止音频输出设备
	 *			将释放start中所申请的资源
	 **/
	virtual void stop() = 0;

	/**
	 *	@name	write
	 *	@brief	将音频数据写入播放设备的缓存buffer
	 *			写入时将不再检查数据的真实音频格式，调用者必须确保写入的数据与设备支持的音频数据格式一直
	 *			
	 *	@param[in]	const uint8_t* buffer 将写入的音频数据首地址
	 *	@param[in]	size_t count 写入的数据的长度
	 *				调用者确保每次写入的数据长度为一个音频frame (frame=sizeof(sample)*channel) 的字节数的整数倍，否则写入失败
	 *	@return		返回真实写入的数据的字节长度
	 **/
	virtual int write(const uint8_t* buffer, size_t count) = 0;

	/**
	 *	@name		getDevInfo
	 *	@brief		调用start成功之后，可使用这个接口获取到设备的信息
	 **/
	const AudioDevInfo* getDevInfo() const { return &m_audioDevInfo; }
	/**
	*	@name		getWaveFormat
	*	@brief		调用start成功之后，可使用这个接口获取打开设备的支持播放的音频数据的WAVEFORMATEX
	*				用户在调用read之前应该先调用这个接口获取音频数据的格式
	**/
	const WAVEFORMATEX& getWaveFormat() const { return m_fmt; }

	/**
	 *	@name		getBufferFrameSize
	 *	@brief		调用start成功之后，调用此接口获取设备的数据缓存队列长度，单位为Frame，frame = sizeof(sample) * channel
	 *				如果为start或者start失败，则该值为0
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