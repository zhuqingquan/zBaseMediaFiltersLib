#include "AudioOutputerCAA.h"
#include "AudioCoreApiHelper.h"
#include <stdio.h>

using namespace zMedia;

zMedia::AudioOutputerCAA::~AudioOutputerCAA()
{
	stop();
}

int zMedia::AudioOutputerCAA::start(const AudioDevInfo* auDevInfo)
{
	if (auDevInfo == nullptr)
		return -1;

	IAudioClient* pAuclient = nullptr;
	HRESULT hr = getIAudioClient(auDevInfo->strDevID, &pAuclient);
	if (FAILED(hr))
	{
		return -2;
	}
	// get format for output data
	WAVEFORMATEX* mixFmt = nullptr;
	hr = pAuclient->GetMixFormat(&mixFmt);
	if (FAILED(hr) || nullptr == mixFmt)
		return -3;
	//fixme 此处使用short未成功
	//WAVEFORMATEXTENSIBLE* mixFmtExt = nullptr;
	//if (mixFmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	//	mixFmtExt = (WAVEFORMATEXTENSIBLE*)mixFmt;
	//if (mixFmtExt != nullptr)
	//{
	//	mixFmtExt->Format.nBlockAlign = 2 * 2;
	//	mixFmtExt->Format.wBitsPerSample = 16;
	//	mixFmtExt->Format.nAvgBytesPerSec = 2 * 2 * mixFmt->nSamplesPerSec;
	//	mixFmtExt->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
	//}
	//else
	//{
	//	mixFmt->wBitsPerSample = 16;
	//	mixFmt->nAvgBytesPerSec = 2 * 2 * mixFmt->nSamplesPerSec;
	//}

	//WAVEFORMATEX* fmt_Active = mixFmt;
	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_MILLISEC * 120;	//buffer大小只有120ms
	hr = pAuclient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		0,
		hnsRequestedDuration,
		0,  //shared mode此参数为0
		mixFmt,
		NULL);
	if (FAILED(hr))
		return -5;

	hr = pAuclient->GetBufferSize(&m_bufferSize);
	if (FAILED(hr))	return -6;
	hr = pAuclient->GetService(__uuidof(IAudioRenderClient), (void**)&m_renderClient);
	if (FAILED(hr)) return -8;

	BYTE* pData = nullptr;
	// Grab all the available space in the shared buffer.
	hr = m_renderClient->GetBuffer(m_bufferSize, &pData);
	if (FAILED(hr) || pData == nullptr)
		return -4;

	uint32_t bytesWrite = m_bufferSize * mixFmt->wBitsPerSample / 8 * mixFmt->nChannels;
	memset(pData, 0, bytesWrite);

	hr = m_renderClient->ReleaseBuffer(m_bufferSize, 0);
	if (FAILED(hr)) return -5;

	hr = pAuclient->Start();
	if (FAILED(hr)) return -7;
	m_fmt = *mixFmt;		// 如果fmt_Active时WAVEFORMATEXTENSIBLE等结构，则这个复制可能有问题，但是如果只是显示channel、samplerate则可能没问题
	m_audioDevInfo = *auDevInfo;
	m_audioClient = pAuclient;
	return 0;
}

void zMedia::AudioOutputerCAA::stop()
{
	if (nullptr == m_audioClient)	return;
	m_audioDevInfo = AudioDevInfo();
	memset(&m_fmt, 0, sizeof(m_fmt));
	HRESULT hr = m_audioClient->Stop();
	if (m_renderClient)
	{
		m_renderClient->Release();
		m_renderClient = nullptr;
	}
	m_audioClient->Release();
	m_audioClient = nullptr;
}

int zMedia::AudioOutputerCAA::write(const uint8_t* buffer, size_t count)
{
	if (buffer == nullptr || count <= 0)
		return -1;
	int frameSize = m_fmt.nChannels * (m_fmt.wBitsPerSample / 8);
	if (count % frameSize != 0)
		return -2;
	if (m_renderClient == nullptr)
		return -3;

	uint32_t numFramesPadding = 0;
	// See how much buffer space is available.
	HRESULT hr = m_audioClient->GetCurrentPadding(&numFramesPadding);
	if (FAILED(hr)) return -3;

	uint32_t numFramesAvailable = m_bufferSize - numFramesPadding;
	if (numFramesAvailable == 0)
	{
		printf("Not free buffer available.\n");
		return 0;
	}

	BYTE* pData = nullptr;
	// Grab all the available space in the shared buffer.
	hr = m_renderClient->GetBuffer(numFramesAvailable, &pData);
	if (FAILED(hr) || pData == nullptr)
		return -4;

	uint32_t bytesWrite = numFramesAvailable * frameSize <= count ? numFramesAvailable * frameSize : count;
	memcpy(pData, buffer, bytesWrite);

	uint32_t frameWrited = bytesWrite / frameSize;
	printf("valid frames %u frameWrited %u\n", numFramesAvailable, frameWrited);

		// Get next 1/2-second of data from the audio source.
	//	hr = pMySource->LoadData(numFramesAvailable, pData, &flags);
	//EXIT_ON_ERROR(hr)

	hr = m_renderClient->ReleaseBuffer(frameWrited, 0);
	if (FAILED(hr)) return -5;

	return frameWrited;
}

const char* zMedia::AudioOutputerCAA::id() const
{
	return nullptr;// m_audioDevInfo.strDevID;
}
