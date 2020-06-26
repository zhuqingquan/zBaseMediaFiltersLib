#include "AudioDevManager.h"
#define INITGUID
#include <mmdeviceapi.h>	//core audio api
#include <DSound.h>
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

int AudioDevManager::getAudioCaptureDevList(AudioDevInfo* pCapDevList, size_t& count)
{
	size_t originCount = count;
	int ds_count = 0, coreAudioApi_Count = 0;
	if (0 > (ds_count = getAudioCaptureDevList_DirectSound(pCapDevList, count)))
	{
		ds_count = 0;
	}
	size_t curCountFree = originCount - ds_count;
	if (0 > (coreAudioApi_Count = getAudioDevList_CoreAudioAPI(pCapDevList+ ds_count, curCountFree, eCapture, DEVICE_STATE_ACTIVE)))
	{
		coreAudioApi_Count = 0;
	}
	count = ds_count + coreAudioApi_Count;
	return ds_count + coreAudioApi_Count;
}

////////// Get Audio Capture Device List with DirectSound ////////////////////
struct AudioCaptureDevInfoList
{
	AudioDevInfo* pCapDevList;
	size_t count;	//pCapDevList的总长度
	size_t size;	//赋值的信息个数
	int index;		//当前的index

	AudioCaptureDevInfoList(AudioDevInfo* _pCapDevList, size_t _count, int _index)
		: pCapDevList(_pCapDevList), count(_count), index(_index), size(0)
	{}
};

BOOL CALLBACK DSEnumProc(LPGUID guid, LPCTSTR desc, LPCTSTR drv, LPVOID obj)
{
	if (guid == nullptr || GUID_NULL == *guid)
	{
		// 此处过滤掉guid为0的默认设备，这个是其他设备的重复
		return TRUE;
	}
	AudioCaptureDevInfoList *pInfoList = (AudioCaptureDevInfoList*)obj;
	if (NULL == pInfoList || pInfoList->index >= (int)pInfoList->count - 1)
		return FALSE;
	AudioDevInfo* devInfo = pInfoList->pCapDevList + pInfoList->index;
	if (devInfo == NULL)
		return FALSE;
	devInfo->set(guid, desc, drv);
	pInfoList->index++;
	pInfoList->size++;
	return TRUE;
}

// DirectSound default capture device GUID {DEF00001-9C6D-47ED-AAF1-4DDA8F2B5C03}
const GUID DEF_DSDEVID_DefaultCapture = { 0xdef00001, 0x9c6d, 0x47ed,{ 0xaa, 0xf1,  0x4d,  0xda,  0x8f,  0x2b,  0x5c,  0x03 } };
//DEFINE_GUID(DEF_DSDEVID_DefaultCapture, 0xdef00001, 0x9c6d, 0x47ed, 0xaa, 0xf1, 0x4d, 0xda, 0x8f, 0x2b, 0x5c, 0x03);

int AudioDevManager::getAudioCaptureDevList_DirectSound(AudioDevInfo* pCapDevList, size_t& count)
{
	if (pCapDevList == NULL || count <= 0)
		return -1;
	GUID defaultCapDevID = { 0 };
	HRESULT hr = GetDeviceID(&DEF_DSDEVID_DefaultCapture, &defaultCapDevID);
	AudioCaptureDevInfoList tmpInfoList(pCapDevList, count, 0);
	if (DirectSoundCaptureEnumerate(DSEnumProc, (LPVOID)&tmpInfoList) == DS_OK)
	{
		if (tmpInfoList.size <= 0)
		{
			return -2;
		}
		count = tmpInfoList.size;
		if (hr == S_OK)
		{
			// 如果默认音频捕捉设备获取成功，设置设备的bPrimary属性
			for (size_t i = 0; i < tmpInfoList.size; i++)
			{
				AudioDevInfo* devInfo = pCapDevList + i;
				if (nullptr == devInfo)	continue;
				if (devInfo->guid == defaultCapDevID)
					devInfo->bPrimary = true;
			}
		}
		return tmpInfoList.size;
	}
	return -3;
}

int zMedia::AudioDevManager::getAudioDevList_CoreAudioAPI(AudioDevInfo * pCapDevList, size_t & count, /*EDataFlow*/DWORD devDataFlow, DWORD stateMask)
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
	hr = pDevEnum->EnumAudioEndpoints((EDataFlow)devDataFlow, /*DEVICE_STATE_ACTIVE*/stateMask, &pDevCol);
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
	hr = pDevEnum->GetDefaultAudioEndpoint((EDataFlow)devDataFlow, eMultimedia, &defaultDev);
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
			//_tcprintf(L"Endpoint %d: \"%ls\" (%ls)\n", i, varName.pwszVal, devID);

			AudioDevInfo* devInfo = tmpInfoList.pCapDevList + tmpInfoList.index;
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

int zMedia::AudioDevManager::getAudioOutputDevList(AudioDevInfo* pCapDevList, size_t& count)
{
	return getAudioDevList_CoreAudioAPI(pCapDevList, count, eRender, DEVICE_STATE_ACTIVE);
}