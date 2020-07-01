#pragma once
#ifndef _Z_MEDIA_AUDIO_CORE_API_HELPER_H_
#define _Z_MEDIA_AUDIO_CORE_API_HELPER_H_

#include <mmdeviceapi.h>	//core audio api
#include <Audioclient.h>

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

namespace zMedia
{
	// 获取与设备ID对应的IAudioClient对象
	HRESULT getIAudioClient(const LPCWSTR devID, IAudioClient** auclient);

}

#endif//_Z_MEDIA_AUDIO_CORE_API_HELPER_H_