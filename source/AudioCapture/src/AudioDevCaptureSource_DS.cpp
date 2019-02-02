#include "AudioDevCaptureSource_DS.h"
#include <dsound.h>

#pragma comment(lib, "Dsound.lib")

#define BUFFER_COUNT_DURATION 80	// buffer中保存数据的总时长，80ms

using namespace zMedia;

AudioDevCaptureSource_DS::AudioDevCaptureSource_DS(const char* id)
: AudioDevCaptureSource(id)
, m_id(id ? id : "")
, m_capture(NULL), m_buffer(NULL)
, m_lastReadOffset(0)
{
	memset(&m_fmt, 0, sizeof(WAVEFORMATEX));
}

AudioDevCaptureSource_DS::~AudioDevCaptureSource_DS()
{
	stop();
}

//bool AudioDevCaptureSource_DS::isValidWaveFmt(const WAVEFORMATEX& waveFmt) const
//{
//	return false;
//}

int AudioDevCaptureSource_DS::start(const AudioCaptureDevInfo* auDevInfo, const WAVEFORMATEX& waveFmt)
{
	if (auDevInfo == NULL || (IsEqualGUID(auDevInfo->guid, GUID_NULL) && !auDevInfo->bPrimary))
		return -1;
//	if (!isValidWaveFmt(waveFmt))
//		return -2;
	if (m_capture || m_buffer)
		return -2;

	HRESULT hr;
	hr = DirectSoundCaptureCreate(&auDevInfo->guid, &this->m_capture, NULL);
	if (hr != DS_OK)
	{
		switch (hr)
		{
		case DSERR_INVALIDPARAM:
			return -5;
		case DSERR_NOAGGREGATION:
			return -6;
		case DSERR_OUTOFMEMORY:
			return -7;
		default:
			return -4;
		}
	}

	//WAVEFORMATEX format;
	//ZeroMemory(&format, sizeof(format));
	//format.wFormatTag = WAVE_FORMAT_PCM;
	//format.nChannels = kChannels;
	//format.nSamplesPerSec = kSampleRate;
	//format.nBlockAlign = kBitsPerSample * kChannels / 8;
	//format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
	//format.wBitsPerSample = kBitsPerSample;

	int bufferBytes = waveFmt.nBlockAlign * (waveFmt.nSamplesPerSec * BUFFER_COUNT_DURATION) / 1000;	//buffer中只保存80ms的数据

	DSCBUFFERDESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.dwSize = sizeof(desc);
	desc.dwBufferBytes = bufferBytes;
	desc.lpwfxFormat = (LPWAVEFORMATEX)&waveFmt;

	hr = m_capture->CreateCaptureBuffer(&desc, &this->m_buffer, NULL);
	if (DS_OK != hr)
	{
		stop();
		switch (hr)
		{
		case DSERR_INVALIDPARAM:
			return -11;
		case DSERR_BADFORMAT:
			return -12;
		case DSERR_GENERIC:
			return -13;
		case DSERR_NODRIVER:
			return -14;
		case DSERR_OUTOFMEMORY:
			return -15;
		case DSERR_UNINITIALIZED:
			return -16;
		default:
			return -10;
		}
	}
	hr = m_buffer->Start(DSCBSTART_LOOPING);
	if (DS_OK != hr)
	{
		stop();
		switch (hr)
		{
		case DSERR_INVALIDPARAM:
			return -21;
		default:
			return -20;
		}
	}
	m_fmt = waveFmt;
	return 0;
}

void AudioDevCaptureSource_DS::stop()
{
	if (m_buffer)
	{
		m_buffer->Stop();
		m_buffer->Release();
		m_buffer = NULL;
	}
	if (m_capture)
	{
		m_capture->Release();
		m_capture = NULL;
	}
	memset(&m_fmt, 0, sizeof(WAVEFORMATEX));
}

int AudioDevCaptureSource_DS::read(const uint8_t* buffer, size_t count, int index)
{
	if (!m_buffer || !m_capture)
		return -1;
	if (buffer == NULL || count <= 0 || index > (int)count)
		return -2;
	int freedBytes = count - index;
	if (freedBytes < m_fmt.nBlockAlign)
	{
		// 每次至少需要拷贝一个sample
		return -3;
	}

	DWORD rpos = 0;
	DWORD n1 = 0, n2 = 0;
	LPVOID ptr1 = NULL, ptr2 = NULL;

	DSCBCAPS bufCaps = { 0 };
	bufCaps.dwSize = sizeof(bufCaps);
	m_buffer->GetCaps(&bufCaps);
	DWORD bufferCount = bufCaps.dwBufferBytes;

	//HRESULT hr = m_buffer->GetCurrentPosition(&cpos, &lrpos);
	int copyed = 0;
	HRESULT	hr = m_buffer->GetCurrentPosition(/*&cpos*/NULL, &rpos);
	printf("Last offset %d read offset %d\n", m_lastReadOffset, rpos);
	freedBytes = freedBytes - (freedBytes % m_fmt.nBlockAlign);	//读取时需要按sample对齐
	int lockedBytes = rpos - m_lastReadOffset;
	if (lockedBytes < 0) lockedBytes += bufferCount;
	lockedBytes = lockedBytes > freedBytes ? freedBytes : lockedBytes;

	hr = m_buffer->Lock(/*rpos*/m_lastReadOffset, lockedBytes, &ptr1, &n1, &ptr2, &n2, 0);
	if (hr == DS_OK)
	{
		int n1_read = 0;
		int n2_read = 0;
		if (ptr1 && n1 > 0 && n1 <= freedBytes)
		{
			memcpy((uint8_t*)buffer + index, ptr1, n1);
			index += n1;
			freedBytes -= n1;
			copyed += n1;
			n1_read = n1;
			m_lastReadOffset += n1;
			m_lastReadOffset %= bufferCount;
		}
		if (ptr2 && n2 > 0 && n2 <= freedBytes)
		{
			memcpy((uint8_t*)buffer + index, ptr2, n2);
			index += n2;
			freedBytes -= n2;
			copyed += n2;
			n2_read = n2;
			m_lastReadOffset += n2;
			m_lastReadOffset %= bufferCount;
		}

		m_buffer->Unlock(ptr1, n1_read, ptr2, n2_read);
	}
	return copyed;
}