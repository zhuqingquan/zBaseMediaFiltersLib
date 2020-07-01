#pragma once
#ifndef _Z_MEDIA_AUDIO_OUTPUTER_CAA_H_
#define _Z_MEDIA_AUDIO_OUTPUTER_CAA_H_

#include "AudioOutputer.h"

struct IAudioClient;
struct IAudioRenderClient;

namespace zMedia
{
class AUDIO_CAPTURE_EXPORT_IMPORT AudioOutputerCAA : public AudioOutputer
{
public:
	virtual ~AudioOutputerCAA();

	// Í¨¹ý AudioOutputer ¼Ì³Ð
	virtual int start(const AudioDevInfo* auDevInfo) override;
	virtual void stop() override;
	virtual int write(const uint8_t* buffer, size_t count) override;
	virtual const char* id() const;
private:
	IAudioClient* m_audioClient = nullptr;
	IAudioRenderClient* m_renderClient = nullptr;
};
}

#endif//_Z_MEDIA_AUDIO_OUTPUTER_CAA_H_