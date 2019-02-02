#include "AudioDevManager.h"
#include <DSound.h>
#define INITGUID
#include <mmdeviceapi.h>	//core audio api
#include <functiondiscoverykeys.h>
#include "TextHelper.h"
#include <tchar.h>

#pragma comment(lib, "Dsound.lib")

using namespace zMedia;

AudioDevManager::AudioDevManager()
{

}

AudioDevManager::~AudioDevManager()
{

}

int AudioDevManager::getAudioCaptureDevList(AudioCaptureDevInfo* pCapDevList, size_t& count)
{
	size_t originCount = count;
	int ds_count = 0, coreAudioApi_Count = 0;
	if (0 > (ds_count = getAudioCaptureDevList_DirectSound(pCapDevList, count)))
	{
		ds_count = 0;
	}
	size_t curCountFree = originCount - count;
	if (0 > (coreAudioApi_Count = getAudioCaptureDevList_CoreAudioAPI(pCapDevList+count, curCountFree)))
	{
		coreAudioApi_Count = 0;
	}
	count = ds_count + coreAudioApi_Count;
	return ds_count + coreAudioApi_Count;
}

////////// Get Audio Capture Device List with DirectSound ////////////////////
struct AudioCaptureDevInfoList
{
	AudioCaptureDevInfo* pCapDevList;
	size_t count;	//pCapDevList的总长度
	size_t size;	//赋值的信息个数
	int index;		//当前的index

	AudioCaptureDevInfoList(AudioCaptureDevInfo* _pCapDevList, size_t _count, int _index)
		: pCapDevList(_pCapDevList), count(_count), index(_index), size(0)
	{}
};

BOOL CALLBACK DSEnumProc(LPGUID guid, LPCTSTR desc, LPCTSTR drv, LPVOID obj)
{
	AudioCaptureDevInfoList *pInfoList = (AudioCaptureDevInfoList*)obj;
	if (NULL == pInfoList || pInfoList->index >= (int)pInfoList->count - 1)
		return FALSE;
	AudioCaptureDevInfo* devInfo = pInfoList->pCapDevList + pInfoList->index;
	if (devInfo == NULL)
		return FALSE;
	devInfo->set(guid, desc, drv);
	pInfoList->index++;
	pInfoList->size++;
	return TRUE;
}

int AudioDevManager::getAudioCaptureDevList_DirectSound(AudioCaptureDevInfo* pCapDevList, size_t& count)
{
	if (pCapDevList == NULL || count <= 0)
		return -1;
	AudioCaptureDevInfoList tmpInfoList(pCapDevList, count, 0);
	if (DirectSoundCaptureEnumerate(DSEnumProc, (LPVOID)&tmpInfoList) == DS_OK)
	{
		if (tmpInfoList.size <= 0)
		{
			return -2;
		}
		count = tmpInfoList.size;
		return tmpInfoList.size;
	}
	return -3;
}

int zMedia::AudioDevManager::getAudioCaptureDevList_CoreAudioAPI(AudioCaptureDevInfo * pCapDevList, size_t & count)
{
	if (pCapDevList == NULL || count <= 0)
		return -1;
	CoInitialize(nullptr);
	AudioCaptureDevInfoList tmpInfoList(pCapDevList, count, 0);
	IMMDeviceEnumerator* pDevEnum = nullptr;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
								  __uuidof(IMMDeviceEnumerator), (LPVOID*)&pDevEnum);
	if (hr != S_OK || nullptr==pDevEnum)
	{
		return -2;
	}
	IMMDeviceCollection* pDevCol = nullptr;
	hr = pDevEnum->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pDevCol);
	if (hr != S_OK || nullptr == pDevCol)
	{
		pDevEnum->Release();
		return -3;
	}
	UINT devCount = 0;
	hr = pDevCol->GetCount(&devCount);
	if (hr != S_OK)
	{
		pDevCol->Release();
		pDevEnum->Release();
		return -4;
	}
	if (0 >= devCount)
	{
		pDevCol->Release();
		pDevEnum->Release();
		return 0;
	}
	IMMDevice* defaultDev = nullptr;
	hr = pDevEnum->GetDefaultAudioEndpoint(eCapture, eMultimedia, &defaultDev);
	LPWSTR defaultDevId = nullptr;
	defaultDev->GetId(&defaultDevId);
	for (UINT i = 0; i < devCount; i++)
	{
		IMMDevice* dev = nullptr;
		IMMEndpoint* endpoint = nullptr;
		hr = pDevCol->Item(i, &dev);
		if (hr != S_OK || nullptr == dev)
		{
			continue;
		}
		hr = dev->QueryInterface(__uuidof(IMMEndpoint), (void**)&endpoint);
		if (hr != S_OK || nullptr == endpoint)
		{
			continue;
		}

		LPWSTR devID = nullptr;
		dev->GetId(&devID);
		DWORD devState;
		dev->GetState(&devState);
		IPropertyStore* pProps = nullptr;
		hr = dev->OpenPropertyStore(STGM_READ, &pProps);
		if (pProps)
		{
			PROPVARIANT varName, varStrGUID, varAssociation, varDevDesc, varAdapterName;
			// Initialize container for property value.
			PropVariantInit(&varName);
			PropVariantInit(&varStrGUID);
			PropVariantInit(&varAssociation);
			PropVariantInit(&varDevDesc);
			PropVariantInit(&varAdapterName);

			// Get the endpoint's friendly-name property.
			hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
			hr = pProps->GetValue(PKEY_AudioEndpoint_GUID, &varStrGUID);
			hr = pProps->GetValue(PKEY_AudioEndpoint_Association, &varAssociation);
			hr = pProps->GetValue(PKEY_Device_DeviceDesc, &varDevDesc);
			hr = pProps->GetValue(PKEY_DeviceInterface_FriendlyName, &varAdapterName);

			// Print endpoint friendly name and endpoint ID.
			_tcprintf(L"Endpoint %d: \"%ls\" (%ls)\n", i, varName.pwszVal, devID);

			AudioCaptureDevInfo* devInfo = tmpInfoList.pCapDevList + tmpInfoList.index;
			if (devInfo == NULL)
				continue;
			GUID guid;
			zUtils::str2Guid(varStrGUID.pwszVal, guid);
			devInfo->set(&guid, devID, varName.pwszVal, nullptr);
			tmpInfoList.index++;
			tmpInfoList.size++;

			if (wcscmp(devID, defaultDevId) == 0)
			{
				devInfo->bPrimary = true;
			}

			PropVariantClear(&varName);
			PropVariantClear(&varStrGUID);
			PropVariantClear(&varAssociation);
			PropVariantClear(&varDevDesc);
			PropVariantClear(&varAdapterName);
		}
		CoTaskMemFree(devID);

		endpoint->Release();
		dev->Release();
	}
	pDevCol->Release();
	pDevEnum->Release();
	count = tmpInfoList.size;
	return tmpInfoList.size;
}
