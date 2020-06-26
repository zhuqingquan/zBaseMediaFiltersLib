#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include "AudioDevManager.h"
#include "AudioDevCaptureSource_DS.h"
#include "AudioDevCaptureSrc_CAA.h"
#include "TextHelper.h"
#include <fstream>

using namespace zMedia;

int main(int argc, _TCHAR* argv[])
{
	size_t capDevCount = 64;
	zMedia::AudioDevInfo* pCapDevList = new AudioDevInfo[capDevCount];
	zMedia::AudioDevManager devManager;
	int ret = devManager.getAudioCaptureDevList(pCapDevList, capDevCount);
	if (ret <= 0)
	{
		printf("Get Audio capture device faile.\n");
		//return -1;
	}
	printf("Audio Capture device:\n");
	for (size_t i = 0; i < capDevCount; i++)
	{
		AudioDevInfo* devInfo = pCapDevList + i;
		if (devInfo)
		{
			std::wstring strGUID = zUtils::guid2Str(&devInfo->guid);
			_tcprintf(L"DEV %d \n\tName=[%ls] primary=%d \n\tGUID=%ls \n\tID=%ls \n\tModuleNmae=%ls\n", 
				i, devInfo->strDescription, devInfo->bPrimary, strGUID.c_str(), devInfo->strDevID, devInfo->strModule);
		}
	}
	printf("------------------------------\n");


	printf("Audio output device:\n");
	size_t outputDevCount = 64;
	zMedia::AudioDevInfo* pOutputDevList = new AudioDevInfo[outputDevCount];
	ret = devManager.getAudioOutputDevList(pOutputDevList, outputDevCount);
	if (ret <= 0)
	{
		printf("Get Audio output device faile.\n");
	}
	for (size_t i = 0; i < outputDevCount; i++)
	{
		AudioDevInfo* devInfo = pOutputDevList + i;
		if (devInfo)
		{
			std::wstring strGUID = zUtils::guid2Str(&devInfo->guid);
			_tcprintf(L"DEV %d \n\tName=[%ls] primary=%d \n\tGUID=%ls \n\tID=%ls \n\tModuleNmae=%ls\n",
				i, devInfo->strDescription, devInfo->bPrimary, strGUID.c_str(), devInfo->strDevID, devInfo->strModule);
		}
	}
	printf("------------------------------\n");


	system("pause");

	std::vector<zMedia::AudioDevCaptureSource*> srcList(capDevCount, nullptr);
	AudioDevCaptureSource* psrc = nullptr;
	AudioDevInfo* pdevInfo = nullptr;
	for (size_t i = 0; i < srcList.size(); i++)
	{
		AudioDevInfo* devInfo = pCapDevList + i;
		if(devInfo->strDevID!=nullptr)
		{
			srcList[i] = new zMedia::AudioDevCaptureSrc_CAA("");
			if (psrc == nullptr)
			{
				psrc = srcList[i];
				pdevInfo = devInfo;
			}
		}
		else if(GUID_NULL!=devInfo->guid || (devInfo->bPrimary && GUID_NULL == devInfo->guid))
			srcList[i] = new zMedia::AudioDevCaptureSource_DS("");
	}
	for (size_t i = 0; i < capDevCount; i++)
	{
		AudioDevInfo* devInfo = pCapDevList + i;
		if (devInfo)
		{
			_tcprintf(_T("DEV %ls:\n"), devInfo->strDescription);
			std::vector<WAVEFORMATEX> waveFmtList = srcList[i]->getSurpportFormat(devInfo);
			if(waveFmtList.size()<=0)
			{
				_tcprintf(_T("Get Dev default surpported format failed.\n"));
			}
			else
			{
				for(size_t j=0; j<waveFmtList.size(); j++)
					_tcprintf(L"\tDev default surpported format: ch=%d sampleRate=%d bitPersample=%d\n",
						waveFmtList[j].nChannels, waveFmtList[j].nSamplesPerSec, waveFmtList[j].wBitsPerSample);
			}
		}
	}

	if (psrc)
	{
		std::ofstream pcmStream("aucap.pcm", std::ios::binary);

		WAVEFORMATEX waveFmt = { 0 };
		waveFmt.cbSize = 0;
		waveFmt.wFormatTag = WAVE_FORMAT_PCM;
		waveFmt.nChannels = 2;
		waveFmt.nSamplesPerSec = 48000;
		waveFmt.wBitsPerSample = 16;
		waveFmt.nBlockAlign = waveFmt.nChannels * waveFmt.wBitsPerSample / 8;
		waveFmt.nAvgBytesPerSec = waveFmt.nBlockAlign * waveFmt.nSamplesPerSec;
		int ret = psrc->start(pdevInfo, waveFmt);
		if (0!=ret)
		{
			printf("Start dev capture failed. %d\n", ret);
			goto APPEND;
		}
		waveFmt = psrc->getWaveFormat();
		printf("Active audio format: ch=%d samplrrate=%u bits=%d\n", waveFmt.nChannels, waveFmt.nSamplesPerSec, waveFmt.wBitsPerSample);
		int buflen = waveFmt.nAvgBytesPerSec * 40 / 1000;
		uint8_t* pbuffer = (uint8_t*)malloc(buflen);
		int index = 0;
		while (true)
		{
			Sleep(1);
			int copyed = psrc->read(pbuffer, buflen, index);
			if (copyed > 0)
			{
				// write to file
				if (pcmStream)
				{
					pcmStream.write((char*)pbuffer, copyed);
				}
				index += copyed;
				index %= buflen;
			}
			else if(copyed<0)
			{
				if (copyed == -11)
				{
					printf("reset buffer offset.\n");
					index = 0;
				}
				printf("Error in read PCM data from audio capture source. %d\n", copyed);
			}
		}
	}

	system("pause");
APPEND:
	for (size_t i = 0; i < srcList.size(); i++)
	{
		delete srcList[i];
	}
	srcList.clear();
	delete[] pCapDevList;
	return 0;
}