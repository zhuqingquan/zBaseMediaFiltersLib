/**
*	@file		AudioDev.h
*	@author		zhuqingquan
*	@brief		音频设备基础结构体定义
*	@created	2019/01/21
**/

#pragma once
#ifndef _AUDIO_DEV_STRUCTURE_DEFINE_H_
#define _AUDIO_DEV_STRUCTURE_DEFINE_H_

#ifdef _WINDOWS
#ifdef AUDIO_CAPTURE_DLL_EXPORT
#define AUDIO_CAPTURE_EXPORT_IMPORT __declspec(dllexport)
#else
#define AUDIO_CAPTURE_EXPORT_IMPORT __declspec(dllimport)
#endif//AUDIO_CAPTURE_DLL_EXPORT
#else //_WINDOWS
#define AUDIO_CAPTURE_EXPORT_IMPORT
#endif//_WINDOWS
#include <Windows.h>

namespace zMedia
{

struct AudioDevInfo
{
	GUID	 guid;
	wchar_t* strDescription;
	wchar_t* strModule;
	wchar_t* strDevID;			// 专门用于保存通过Core Audio API获取到的Device ID。通过DS接口获取到的设备信息中没有这个信息
	bool	 bPrimary;

	AudioDevInfo()
		: strDescription(NULL), strModule(NULL), bPrimary(false), strDevID(NULL)
	{
		memset(&guid, 0, sizeof(GUID));
	}

	AudioDevInfo(LPGUID lpGuid, LPCTSTR lpcstrDesc, LPCTSTR lpcstrModule)
		: strDescription(NULL), strModule(NULL), bPrimary(false)
	{
		memset(&guid, 0, sizeof(GUID));
		set(lpGuid, lpcstrDesc, lpcstrModule);
	}

	void set(LPGUID lpGuid, LPCTSTR lpcstrDesc, LPCTSTR lpcstrModule)
	{
		int len = 0;
		if (lpcstrDesc && (len=lstrlen(lpcstrDesc)) > 0)
		{
			strDescription = (wchar_t*)malloc((len+1) * sizeof(wchar_t));
			lstrcpy(strDescription, lpcstrDesc);
			strDescription[len] = '\0';
		}
		if (lpcstrModule && (len = lstrlen(lpcstrModule)) > 0)
		{
			strModule = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
			lstrcpy(strModule, lpcstrModule);
			strModule[len] = '\0';
		}
		if (lpGuid) guid = *lpGuid;
		bPrimary = lpGuid == NULL;
	}

	void set(LPGUID lpGuid, LPCTSTR lpDevID, LPCTSTR lpcstrDesc, LPCTSTR lpcstrModule)
	{
		set(lpGuid, lpcstrDesc, lpcstrModule);
		int len = 0;
		if (lpDevID && (len = lstrlen(lpDevID)) > 0)
		{
			strDevID = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
			lstrcpy(strDevID, lpDevID);
			strDevID[len] = '\0';
		}
	}

	~AudioDevInfo()
	{
		if (strDescription) { free(strDescription); strDescription = NULL; }
		if (strModule) { free(strModule); strModule = NULL; }
	}
};

}
#endif//_AUDIO_DEV_STRUCTURE_DEFINE_H_