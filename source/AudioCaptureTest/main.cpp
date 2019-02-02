#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include "AudioDevManager.h"
#include "TextHelper.h"

using namespace zMedia;

int main(int argc, _TCHAR* argv[])
{
	size_t capDevCount = 64;
	zMedia::AudioCaptureDevInfo* pCapDevList = new AudioCaptureDevInfo[capDevCount];
	zMedia::AudioDevManager devManager;
	int ret = devManager.getAudioCaptureDevList(pCapDevList, capDevCount);
	if (ret <= 0)
	{
		printf("Get Audio capture device faile.\n");
		return -1;
	}
	for (size_t i = 0; i < capDevCount; i++)
	{
		AudioCaptureDevInfo* devInfo = pCapDevList + i;
		if (devInfo)
		{
			std::wstring strGUID = zUtils::guid2Str(&devInfo->guid);
			_tcprintf(L"DEV %d \n\tName=[%ls] primary=%d \n\tGUID=%ls \n\tID=%ls \n\tModuleNmae=%ls\n", 
				i, devInfo->strDescription, devInfo->bPrimary, strGUID.c_str(), devInfo->strDevID, devInfo->strModule);
		}
	}
	system("pause");
	delete[] pCapDevList;
	return 0;
}