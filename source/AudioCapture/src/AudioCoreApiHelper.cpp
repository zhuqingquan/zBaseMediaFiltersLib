#include "AudioCoreApiHelper.h"

using namespace zMedia;

HRESULT zMedia::getIAudioClient(const LPCWSTR devID, IAudioClient** auclient)
{
	IMMDeviceEnumerator* pDevEnum = nullptr;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator), (LPVOID*)&pDevEnum);
	if (hr != S_OK || nullptr == pDevEnum)
	{
		return S_FALSE;
	}
	IMMDevice* pDevice = nullptr;
	hr = pDevEnum->GetDevice(devID, &pDevice);
	if (hr != S_OK || nullptr == pDevice)
	{
		return S_FALSE;
	}
	//IAudioClient* pAuclient = nullptr;
	hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)auclient);
	if (FAILED(hr) || nullptr == *auclient)
	{
		return S_FALSE;
	}
	pDevEnum->Release();
	pDevice->Release();
	return hr;
}