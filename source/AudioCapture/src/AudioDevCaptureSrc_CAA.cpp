#include "AudioDevCaptureSrc_CAA.h"
#include "AudioCoreApiHelper.h"

using namespace zMedia;

AudioDevCaptureSrc_CAA::AudioDevCaptureSrc_CAA(const char * id)
	: AudioDevCaptureSource(id)
	, m_id(id?id:"")
	, m_audioClient(nullptr)
	, m_captureClient(nullptr)
{
}

AudioDevCaptureSrc_CAA::~AudioDevCaptureSrc_CAA()
{
	stop();
}

std::vector<WAVEFORMATEX> AudioDevCaptureSrc_CAA::getSurpportFormat(const AudioDevInfo* auDevInfo)
{
	std::vector<WAVEFORMATEX> result;
	/*if (auDevInfo == nullptr)
		return result;
	IAudioClient* pAuclient = nullptr;
	HRESULT hr = getIAudioClient(auDevInfo->strDevID, &pAuclient);
	if (FAILED(hr))
	{
		return result;
	}
	WAVEFORMATEX* waveFmtList = nullptr;
	hr = pAuclient->GetMixFormat(&waveFmtList);
	if (FAILED(hr))
		return result;
	while (waveFmtList)
	{
		result.push_back(*waveFmtList);
		waveFmtList++;
	}*/
	return result;
}

int AudioDevCaptureSrc_CAA::start(const AudioDevInfo * auDevInfo, const WAVEFORMATEX & waveFmt)
{
	if (auDevInfo == nullptr)
		return -1;

	IAudioClient* pAuclient = nullptr;
	HRESULT hr = getIAudioClient(auDevInfo->strDevID, &pAuclient);
	if (FAILED(hr))
	{
		return -2;
	}
	WAVEFORMATEX* mixFmt = nullptr;
	hr = pAuclient->GetMixFormat(&mixFmt);
	if (FAILED(hr) || nullptr==mixFmt)
		return -3;
	WAVEFORMATEX* tmp = nullptr;
	WAVEFORMATEX* fmt_Active = nullptr;
	bool sharedMode = true;
	// 先尝试判断shared-mode是否支持这种格式
	hr = pAuclient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &waveFmt, &tmp);
	if (hr==S_OK)
	{
		sharedMode = true;
		fmt_Active = const_cast<WAVEFORMATEX*>(&waveFmt);
	}
	else if (hr == S_FALSE && nullptr!=tmp)
	{
		// 不直接支持waveFmt的格式，但是支持一个近似的格式
		sharedMode = true;
		fmt_Active = tmp;
	}
	else
	{
		// 在shared-mode不支持时再去判断下exclusive-mode是否支持
		hr = pAuclient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &waveFmt, nullptr);
		if (AUDCLNT_E_UNSUPPORTED_FORMAT == hr || FAILED(hr))
		{
			// wave format不支持
			CoTaskMemFree(tmp);
			return -4;
		}
	}
	CoTaskMemFree(tmp);
	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_MILLISEC * 120;	//buffer大小只有120ms
	hr = pAuclient->Initialize(
		sharedMode?AUDCLNT_SHAREMODE_SHARED:AUDCLNT_SHAREMODE_EXCLUSIVE,
		0,
		hnsRequestedDuration,
		0,
		//mixFmt,
		fmt_Active,
		NULL);
	if (FAILED(hr))
		return -5;
	UINT32 bufferSize = 0;
	hr = pAuclient->GetBufferSize(&bufferSize);
	if (FAILED(hr))	return -6;
	hr = pAuclient->GetService(__uuidof(IAudioCaptureClient), (void**)&m_captureClient);
	if (FAILED(hr)) return -8;
	hr = pAuclient->Start();
	if (FAILED(hr)) return -7;
	m_fmt = *fmt_Active;		// 如果fmt_Active时WAVEFORMATEXTENSIBLE等结构，则这个复制可能有问题，但是如果只是显示channel、samplerate则可能没问题
	m_fmt.wFormatTag = WAVE_FORMAT_PCM;
	m_fmt.cbSize = 0;
	m_audioDevInfo = *auDevInfo;
	m_audioClient = pAuclient;
	
	return 0;
}

void AudioDevCaptureSrc_CAA::stop()
{
	if (nullptr == m_audioClient)	return;
	m_audioDevInfo = AudioDevInfo();
	memset(&m_fmt, 0, sizeof(m_fmt));
	HRESULT hr = m_audioClient->Stop();
	if (m_captureClient)
	{
		m_captureClient->Release();
		m_captureClient = nullptr;
	}
	m_audioClient->Release();
	m_audioClient = nullptr;
}

int AudioDevCaptureSrc_CAA::read(const uint8_t * buffer, size_t count, int index)
{
	if (buffer == nullptr || count <= 0 || index < 0)
		return -1;
	if (m_captureClient == nullptr || nullptr == m_audioClient)
		return -2;
	size_t freeBytes = count - index;
	UINT32 nextPktSize = 0;
	HRESULT hr = m_captureClient->GetNextPacketSize(&nextPktSize);
	if (FAILED(hr))
	{
		return -3;
	}
	uint8_t* pData = nullptr;
	// Get the available data in the shared buffer.
	DWORD flags = 0;
	UINT32 numFramesAvailable = 0;
	hr = m_captureClient->GetBuffer(&pData,
		&numFramesAvailable,
		&flags, NULL, NULL); 
	if (AUDCLNT_S_BUFFER_EMPTY == hr)
	{
		//m_captureClient->ReleaseBuffer(numFramesAvailable);
		return 0;
	}
	else if (AUDCLNT_E_BUFFER_ERROR == hr)
		return -4;
	else if (AUDCLNT_E_OUT_OF_ORDER == hr)
	{
		//尝试调用一次ReleaseBuffer，或许可以修复
		m_captureClient->ReleaseBuffer(numFramesAvailable);
		return -5;
	}
	else if (AUDCLNT_E_DEVICE_INVALIDATED == hr)
	{
		return -6;
	}
	else if (AUDCLNT_E_BUFFER_OPERATION_PENDING == hr)
	{
		return -7;
	}
	else if (AUDCLNT_E_SERVICE_NOT_RUNNING == hr)
	{
		return -8;
	}
	else if (E_POINTER == hr)
	{
		return -9;
	}
	else if (hr == S_OK && 0 < numFramesAvailable)
	{
		uint32_t copyed = 0;
		if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
		{
			pData = NULL;  // Tell CopyData to write silence.
			printf("data silent\n");
		}
		//else if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY)
		//{
		//	printf("Glitch happened in audio stream.\n");
		//}
		else
		{
			uint32_t availableBytes = numFramesAvailable * m_fmt.nBlockAlign;
			freeBytes = freeBytes - (freeBytes % m_fmt.nBlockAlign);
			if (freeBytes < availableBytes)
			{
				printf("Buffer space is Not enough. freeBytes = %d  need=%u\n", freeBytes, availableBytes);
				m_captureClient->ReleaseBuffer(0);
				return -11;
			}
			copyed = freeBytes >= availableBytes ? availableBytes : freeBytes;
			memcpy((uint8_t*)buffer, pData, copyed);
		}
		hr = m_captureClient->ReleaseBuffer(copyed / m_fmt.nBlockAlign);
		return copyed;
	}

	return -10;
}
