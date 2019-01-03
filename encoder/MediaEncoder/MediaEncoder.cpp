#include "MediaEncoder.h"
#include <Windows.h>
#include <tchar.h>
#include "DwHWVideoEncoder.h"
#include "FFMPEGAudioEncoder.h"
#include "QSVVideoEncoder.h"
#include "x264Encoder.h"
#include "NVSDKVideoEncoder.h"

using namespace Media;

bool checkSurpportEncoder(VIDEO_CODEC_IMPL implIndex);

int GetVideoEncAbility()
{
	int result = 0;
	if(checkSurpportEncoder(HW_CODEC_NVIDIA_NVENC))
	{
		result |= ABILITY_NVIDIA_AVC;
	}
	if(checkSurpportEncoder(HW_CODEC_INTEL_QUICKSYNC))
	{
		result |= ABILITY_INTEL_AVC;
	}
	//if(checkSurpportEncoder(HW_CODEC_NVIDIA_NVENC_HEVC))
	//{
	//	result |= ABILITY_NVIDIA_HEVC;
	//}
	//if(checkSurpportEncoder(HW_CODEC_INTEL_QUICKSYNC_HEVC))
	//{
	//	result |= ABILITY_INTEL_HEVC;
	//}
	return result;
}

int CreateVideoEncoder(VIDEO_CODEC_IMPL codecType, Media::VideoEncoder** outEncoder)
{
	if(outEncoder==NULL)
		return -1;
	switch(codecType)
	{
	case Media::HW_CODEC_INTEL_QUICKSYNC:
		*outEncoder = new Media::QSVVideoEncoder();
		return 0;
	case Media::HW_CODEC_NVIDIA_NVENC:
		*outEncoder = new Media::NVSDKVideoEncoder();
		return 0;
	case Media::HW_CODEC_NVIDIA_NVENC_HEVC:
	case Media::HW_CODEC_INTEL_QUICKSYNC_HEVC:
		return 0;
	case Media::SW_CODEC_X264:
		*outEncoder = new Media::x264Encoder();
		return 0;
	case Media::SW_CODEC_X265:
	default:
		*outEncoder = NULL;
		return -1;
	}
}

int ReleaseVideoEncoder(Media::VideoEncoder** encoder)
{
	if(encoder==NULL || NULL==*encoder)	return -1;
	Media::VideoEncoder* penc = *encoder;
	*encoder = NULL;
	delete penc;
	return 0;
}

int CreateAudioEncoder(Media::AUDIO_ENC_CODEC_ID codecID, Media::AudioEncoder** outEncoder)
{
	if(NULL==outEncoder)	return -1;
	switch(codecID)
	{
	case AUDIO_CODEC_ID_AAC:
	case AUDIO_CODEC_ID_MP3:
		*outEncoder = new Media::FFMPEGAudioEncoder(codecID);
		return 0;
	default:
		*outEncoder = NULL;
		break;
	}
	return -2;
}

int ReleaseAudioEncoder(Media::AudioEncoder** encoder)
{
	if(encoder==NULL || NULL==*encoder)	return -1;
	Media::AudioEncoder* penc = *encoder;
	*encoder = NULL;
	delete penc;
	return 0;
}

bool checkSurpportEncoder(VIDEO_CODEC_IMPL implIndex)
{
	TCHAR cmdline[256] = {0};
	_stprintf_s(cmdline, 256, _T("media_sdk/hwCodecTest.exe %d"), implIndex);
	PROCESS_INFORMATION processInfo;
	STARTUPINFO si = {sizeof(si)};
	memset(&processInfo, 0, sizeof(processInfo));
	//TCHAR* pCmdLine = (TCHAR*)cmdline.c_str();
	BOOL result = CreateProcess(NULL, cmdline, NULL, NULL, false, CREATE_NO_WINDOW, NULL, NULL, &si, &processInfo);
	if(!result)
	{
		DWORD errcode = GetLastError();
		//liblog::Log::e(TAG, libtext::format(_T("CreateProcess failed. cmdline=[%s] Errcode=[%u]"), cmdline.c_str(), errcode));
		return false;
	}
	//m_processID = processInfo.dwProcessId;
	WaitForSingleObject(processInfo.hProcess, 3000);
	DWORD exitCode = -1;
	GetExitCodeProcess(processInfo.hProcess, &exitCode);
	CloseHandle(processInfo.hProcess);
	return exitCode==0;
}
