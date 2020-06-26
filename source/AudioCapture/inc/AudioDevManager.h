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
	 *	@param[in,out]	AudioDevInfo* pCapDevList ����AudioDevInfo�б��ɹ���ȡ�󣬱���������Ƶ��׽�豸��Ϣ
	 *	@param[in,out]	size_t& count ����AudioDevInfo�б�����size������ɹ���ȡ�����豸��Ϣ����
	 *	@return		int >=0--�ɹ���ȡ�����豸��Ϣ���� <0--ʧ��
	 **/
	int getAudioCaptureDevList(AudioDevInfo* pCapDevList, size_t& count);

	/**
	 *	@name		getAudioOutputDevList
	 *	@brief		��ȡ������Ƶ�����豸
	 *	@param[in,out]	AudioDevInfo* pCapDevList ����AudioDevInfo�б��ɹ���ȡ�󣬱���������Ƶ�����豸��Ϣ
	 *	@param[in,out]	size_t& count ����AudioDevInfo�б�����size������ɹ���ȡ�����豸��Ϣ����
	 *	@return		int >=0--�ɹ���ȡ�����豸��Ϣ���� <0--ʧ��
	 **/
	int getAudioOutputDevList(AudioDevInfo* pCapDevList, size_t& count);
private:
	// ʹ��DirectSound�ӿڻ�ȡ������Ƶ��׽�豸
	int getAudioCaptureDevList_DirectSound(AudioDevInfo* pCapDevList, size_t& count);
	// ʹ��Core Audio API�ӿڻ�ȡ������Ƶ��׽�豸
	int getAudioDevList_CoreAudioAPI(AudioDevInfo* pCapDevList, size_t& count, /*EDataFlow*/DWORD devDataFlow, DWORD stateMask);
};

}
#endif //_AUDIO_DEV_MANAGER_H_