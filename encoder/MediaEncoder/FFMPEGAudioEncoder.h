#pragma once
#ifndef _MEDIA_FFMEPG_AUDIO_ENCODER_H_
#define _MEDIA_FFMEPG_AUDIO_ENCODER_H_

#include "MediaEncoder.h"
#include <list>

struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct AVPacket;

class CRingBuffer;

namespace Media
{
	class FFMPEGAudioEncoder : public AudioEncoder
	{
	public:
		FFMPEGAudioEncoder(AUDIO_ENC_CODEC_ID codecID);
		virtual ~FFMPEGAudioEncoder();

		virtual bool init();
		virtual bool setConfig(const AudioEncoderConfig& aCfg);

		virtual bool encodeFrame(zMedia::AudioData::SPtr vframe);
		virtual bool getEncFrame(AudioOutputFrame& outFrame);

	private:
		AUDIO_ENC_CODEC_ID m_codecID;
		AVCodecContext* m_pCodecCtx;  
		AVCodec* m_pCodec;
		AVFrame* m_pFrame;
		int m_frameBytes;
		unsigned int m_lastFrameTs;
		int m_lastFrameDuration;
		unsigned int m_curPTS_ffmpeg;
		unsigned int m_curPTS;
		int64_t m_lastpts;

		std::list<AVPacket*> m_encedPktList;
		CRingBuffer* m_pcmBufCache;
		unsigned char* m_dataWaitForEnc;
	};
}

#endif //_MEDIA_FFMEPG_AUDIO_ENCODER_H_