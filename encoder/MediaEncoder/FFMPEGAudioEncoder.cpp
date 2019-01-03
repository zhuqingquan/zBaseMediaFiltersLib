#define __STDC_CONSTANT_MACROS  

#include <inttypes.h>
#include <stdio.h>  
#include <tchar.h>
#include <fstream>
#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32  
//Windows  
extern "C"  
{ 
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/pixfmt.h>
};  
#else  
//Linux...  
#ifdef __cplusplus  
extern "C"  
{  
#endif  
#include <libavcodec/avcodec.h>  
#include <libavformat/avformat.h>  
#ifdef __cplusplus  
};  
#endif  
#endif  
#include "AudioData.h"
#include "FFMPEGAudioEncoder.h"
#include "ByteRingBuffer.h"
#include "CriticalSection.h"

using namespace Media;

static CCriticalLock g_module_mutex;
int g_bFFmpegInit = 0;
void InitFFmpegMediaLib()
{
	CAutoLock lock(g_module_mutex);
	if (!g_bFFmpegInit)
	{
		av_register_all();
		avcodec_register_all();
		avformat_network_init();
#ifdef _DEBUG
		av_log_set_level(AV_LOG_DEBUG);
#else
		av_log_set_level(AV_LOG_ERROR);
#endif

#ifndef WIN32
		if (av_lockmgr_register(lockmgr))
		{
			logth(Error, "Could not initialize lock manager!\n");
			return;
		}
#endif
		g_bFFmpegInit = 1;
	}
}

Media::FFMPEGAudioEncoder::FFMPEGAudioEncoder(AUDIO_ENC_CODEC_ID codecID)
: m_codecID(codecID)
, m_pCodecCtx(NULL)
, m_pCodec(NULL)
, m_pFrame(NULL)
, m_frameBytes(0)
, m_dataWaitForEnc(NULL)
, m_pcmBufCache(NULL)
, m_lastFrameTs(0xFFFFFFFF)
, m_curPTS(0xFFFFFFFF)
, m_lastFrameDuration(0)
, m_curPTS_ffmpeg(0)
, m_lastpts(0)
{

}

Media::FFMPEGAudioEncoder::~FFMPEGAudioEncoder()
{
	std::list<AVPacket*>::const_iterator iter=m_encedPktList.begin();
	for (; iter!=m_encedPktList.end(); iter++)
	{
		AVPacket* pkt = *iter;
		//av_packet_unref(pkt);
		av_packet_free(&pkt);
	}
	m_encedPktList.clear();

	if(m_dataWaitForEnc)
	{
		free(m_dataWaitForEnc);
		m_dataWaitForEnc = NULL;
	}

	if (m_pFrame)
	{
		av_frame_free(&m_pFrame);
	}
	if (m_pCodecCtx)
	{
		avcodec_free_context(&m_pCodecCtx);
	}

	if(m_pcmBufCache)
	{
		delete m_pcmBufCache;
		m_pcmBufCache = NULL;
	}
	if(m_dataWaitForEnc)
	{
		free(m_dataWaitForEnc);
		m_dataWaitForEnc = NULL;
	}
}

bool Media::FFMPEGAudioEncoder::init()
{
	InitFFmpegMediaLib();
	if(m_pCodec)	return true; //only get encoder once
	switch(m_codecID)
	{
	case AUDIO_CODEC_ID_AAC:
		{
			AVCodec * pEncCodec = avcodec_find_encoder_by_name("libfdk_aac");//libaacplus libfaac libfdk_aac
			if (!pEncCodec)
			{
				pEncCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
				if (!pEncCodec) 
				{
					return false;
				}
			}
			m_pCodec = pEncCodec;
			return true;
		}
		break;
	case AUDIO_CODEC_ID_MP3:
		break;
	default:
		break;
	}
	return false;
}

bool Media::FFMPEGAudioEncoder::setConfig(const AudioEncoderConfig& aCfg)
{
	if(aCfg.codecID!=m_codecID)	return false;
	if(NULL==m_pCodec)	return false;
	if(m_pCodecCtx)	return true;//only call once
	AVCodecContext * pEncCodecCtx = NULL;
	pEncCodecCtx = avcodec_alloc_context3(m_pCodec);
	pEncCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
	pEncCodecCtx->sample_rate = aCfg.nSampleRate;
	pEncCodecCtx->channels = aCfg.nChannels;
	pEncCodecCtx->channel_layout = av_get_default_channel_layout(aCfg.nChannels);
	pEncCodecCtx->bit_rate = aCfg.nBitrate;
	if(m_codecID==AUDIO_CODEC_ID_AAC)
	{
		if(AUDIO_ENC_PROFILE_AAC_HE==aCfg.profile)
		{
			pEncCodecCtx->profile = FF_PROFILE_AAC_HE_V2;
		}
		else if(AUDIO_ENC_PROFILE_AAC_LC==aCfg.profile)
		{
			pEncCodecCtx->profile = FF_PROFILE_AAC_LOW;
		}
		pEncCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;//libfdk_aac 则必须使用S16
	}
	if (avcodec_open2(pEncCodecCtx, m_pCodec,NULL) < 0){  
		printf("Failed to open encoder!\n");  
		avcodec_free_context(&pEncCodecCtx);
		return false;  
	}  
	AVFrame* pFrame = av_frame_alloc();  
	pFrame->nb_samples= pEncCodecCtx->frame_size;  
	pFrame->format= pEncCodecCtx->sample_fmt;  
	int size = av_samples_get_buffer_size(NULL, pEncCodecCtx->channels, pEncCodecCtx->frame_size, pEncCodecCtx->sample_fmt, 1);  

	m_pCodecCtx = pEncCodecCtx;
	m_pFrame = pFrame;
	m_frameBytes = size;
	//初始化缓冲RingBuffer为1s（采样格式为float时）或者2s（采样格式为short时）中音频数据的大小
	m_pcmBufCache = new CRingBuffer(m_pCodecCtx->sample_rate * m_pCodecCtx->channels * 4);
	m_dataWaitForEnc = (unsigned char*)malloc(m_frameBytes);
	m_lastFrameTs = 0xFFFFFFFF;
	m_lastFrameDuration = 0;
	m_curPTS = 0xFFFFFFFF;
	m_curPTS_ffmpeg = 0;
	m_lastpts = 0;
	return true;
}

bool Media::FFMPEGAudioEncoder::encodeFrame(zMedia::AudioData::SPtr aframe)
{
	zMedia::PcmData::SPtr pcmData = aframe->getRawData();
	if(pcmData==NULL || pcmData->size()<=0)	return false;
	if(m_pCodecCtx==NULL)	return false;
	if(pcmData->channels() != m_pCodecCtx->channels || pcmData->sampleRate()!=m_pCodecCtx->sample_rate)
		return false;
	if(AUDIO_CODEC_ID_AAC==m_codecID && pcmData->sampleSize()!=zMedia::AudioSampleSize_SHORT)
		return false;
	if(m_lastFrameTs!=0xFFFFFFFF)
	{
		int dst = pcmData->getTimeStamp()-(m_lastFrameDuration+m_lastFrameTs);
		dst = dst>=0 ? dst : -dst;
		if(dst > 1000)
		{
			//此时数据时间戳可能异常
			//m_curPTS = 0;
			m_pcmBufCache->clear();//清除部分数据不知是否合理？？？
			m_lastFrameTs = 0xFFFFFFFF;
			m_lastFrameDuration = 0;
			m_curPTS = 0xFFFFFFFF;
			m_curPTS_ffmpeg = 0;
			m_lastpts = 0;
		}
	}
	if(0xFFFFFFFF==m_curPTS || m_pcmBufCache->size()<=0)
	{
		m_curPTS = pcmData->getTimeStamp();
		m_curPTS_ffmpeg = 0;
		OutputDebugStringA("---------------音频时间戳矫正。\n");
	}
	m_lastFrameTs = pcmData->getTimeStamp();
	m_lastFrameDuration = pcmData->getTimeCount();//vframe->sampleCount() * 1000 / m_pCodecCtx->sample_rate / vframe->channels;
	m_pcmBufCache->write((char*)pcmData->data(), pcmData->size());
	int readed = 0;
	int got_frame = 0;
	int ret = 0;
	double dTimeBase = av_q2d(m_pCodecCtx->time_base);
	do 
	{
		readed = m_pcmBufCache->read((char*)m_dataWaitForEnc, m_frameBytes);
		if(readed==m_frameBytes)
		{
			AVPacket* pkt = av_packet_alloc();
			av_new_packet(pkt, m_frameBytes);  
			m_pFrame->pts = m_curPTS_ffmpeg;//vframe->getTimestamp();
			m_curPTS_ffmpeg += m_pCodecCtx->frame_size;
			avcodec_fill_audio_frame(m_pFrame, m_pCodecCtx->channels, m_pCodecCtx->sample_fmt, (const uint8_t*)m_dataWaitForEnc, m_frameBytes, 1);
			got_frame=0;  
			//Encode  
			ret = avcodec_encode_audio2(m_pCodecCtx, pkt, m_pFrame, &got_frame);  
			if(ret < 0){  
				av_packet_free(&pkt);
				continue;
			}  
			if (got_frame==1){  
				//printf("Succeed to encode 1 frame! \tsize:%5d\n",pkt->size);  
				//encodeAACHeader(aacHeader, 16, pkt.size, 44100, channel);
				//outFileStream.write((char*)aacHeader, 7);
				//outFileStream.write((char*)pkt.data, pkt.size);
				//            ret = av_write_frame(pFormatCtx, &pkt);  
				pkt->pts = m_curPTS + (DWORD)(dTimeBase * 1000 * m_pFrame->pts);//m_pFrame->pts;
				pkt->dts = pkt->pts;//m_pFrame->pts;
				//m_curPTS += (DWORD)(dTimeBase * 1000 * m_curPTS_ffmpeg);
				m_encedPktList.push_back(pkt);
				if(pkt->pts<=m_lastpts)
				{
					pkt->pts = m_lastpts;
					pkt->dts = m_lastpts;
					OutputDebugStringA("-------------音频时间戳有错误\n");
				}
				m_lastpts = pkt->pts;
			}  
		}
	} while (readed==m_frameBytes);
	return true;
}

bool Media::FFMPEGAudioEncoder::getEncFrame(AudioOutputFrame& outFrame)
{
	if(m_encedPktList.size()<=0)	return false;
	if(outFrame.frameData==NULL || outFrame.frameSize<=0)	return false;
	AVPacket* pkt = m_encedPktList.front();
	if(pkt->data==NULL || (pkt->size>0 && (uint32_t)pkt->size>outFrame.frameSize))
		return false;
	m_encedPktList.pop_front();
	outFrame.codecID = m_codecID;
	//double dTimeBase = av_q2d(m_pCodecCtx->time_base);
	//outFrame.pts = (DWORD)(dTimeBase * 1000 * pkt->pts);
	//outFrame.dts = outFrame.pts;
	outFrame.pts = pkt->pts;
	outFrame.dts = pkt->dts;
	memcpy(outFrame.frameData, pkt->data, pkt->size);
	outFrame.frameSize = pkt->size;
	av_packet_free(&pkt); //av_packet_unref(pkt);
	return true;
}
