/**
 *	@file		AudioDevManager.h
 *	@author		zhuqingquan
 *	@brief		��ȡϵͳ�е�ǰ���е���Ƶ�����豸�б������豸���ʱ�ص�
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
 *	@brief	��Ƶ�豸�����࣬ʵ�ֻ�ȡ������Ƶ�豸
 */
class AUDIO_CAPTURE_EXPORT_IMPORT AudioDevManager
{
public:
	AudioDevManager();
	~AudioDevManager();

	/**
	 *	@name		getAudioCaptureDevList
	 *	@brief		��ȡ������Ƶ��׽�豸
	 *	@param[in,out]	AudioCaptureDevInfo* pCapDevList ����AudioCaptureDevInfo�б��ɹ���ȡ�󣬱���������Ƶ��׽�豸��Ϣ
	 *	@param[in,out]	size_t& count ����AudioCaptureDevInfo�б�����size������ɹ���ȡ�����豸��Ϣ����
	 *	@return		int >=0--�ɹ���ȡ�����豸��Ϣ���� <0--ʧ��
	 **/
	int getAudioCaptureDevList(AudioCaptureDevInfo* pCapDevList, size_t& count);
private:
	// ʹ��DirectSound�ӿڻ�ȡ������Ƶ��׽�豸
	int getAudioCaptureDevList_DirectSound(AudioCaptureDevInfo* pCapDevList, size_t& count);
	// ʹ��Core Audio API�ӿڻ�ȡ������Ƶ��׽�豸
	int getAudioCaptureDevList_CoreAudioAPI(AudioCaptureDevInfo* pCapDevList, size_t& count);
};

}
#endif //_AUDIO_DEV_MANAGER_H_