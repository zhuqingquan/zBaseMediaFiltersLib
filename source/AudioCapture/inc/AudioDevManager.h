/**
 *	@file		AudioDevManager.h
 *	@author		zhuqingquan
 *	@brief		获取系统中当前所有的音频输入设备列表，并在设备插拔时回调
 *	@created	2019/01/21
 **/

#pragma once
#ifndef _AUDIO_DEV_MANAGER_H_
#define _AUDIO_DEV_MANAGER_H_

#include "AudioDev.h"

namespace zMedia
{

/**
 *	@name	AudioDevManager
 *	@brief	音频设备管理类，实现获取所有音频设备
 */
class AUDIO_CAPTURE_EXPORT_IMPORT AudioDevManager
{
public:
	AudioDevManager();
	~AudioDevManager();

	/**
	 *	@name		getAudioCaptureDevList
	 *	@brief		获取所有音频捕捉设备
	 *	@param[in,out]	AudioCaptureDevInfo* pCapDevList 传入AudioCaptureDevInfo列表，成功获取后，保存所有音频捕捉设备信息
	 *	@param[in,out]	size_t& count 传入AudioCaptureDevInfo列表的最大size，传入成功获取到的设备信息个数
	 *	@return		int >=0--成功获取到的设备信息个数 <0--失败
	 **/
	int getAudioCaptureDevList(AudioCaptureDevInfo* pCapDevList, size_t& count);
private:
	// 使用DirectSound接口获取所有音频捕捉设备
	int getAudioCaptureDevList_DirectSound(AudioCaptureDevInfo* pCapDevList, size_t& count);
	// 使用Core Audio API接口获取所有音频捕捉设备
	int getAudioCaptureDevList_CoreAudioAPI(AudioCaptureDevInfo* pCapDevList, size_t& count);
};

}
#endif //_AUDIO_DEV_MANAGER_H_