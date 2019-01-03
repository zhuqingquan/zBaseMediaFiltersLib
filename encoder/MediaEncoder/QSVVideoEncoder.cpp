#include "QSVVideoEncoder.h"
#include "libobssqv11.h"
#include "obs-avc.h"
#include <list>
#include "AudioData.h"

using namespace Media;

namespace Media
{
	struct QSVVideoEncoderPrivate
	{
		qsv_param_t		params;
		qsv_t*			context;
		unsigned short	verMajor; 
		unsigned short	verMinor; 
		uint8_t*		spspps;
		uint32_t		spsppsLen;
		uint64_t		pts;
		std::list<VideoOutputFrame> encodedFrames;

		QSVVideoEncoderPrivate()
			: context(NULL), verMajor(0), verMinor(0)
			, spspps(NULL), spsppsLen(0), pts(0)
		{
			memset(&params, 0, sizeof(qsv_param_t));
		}
	};
}


QSVVideoEncoder::QSVVideoEncoder()
: m_private(new QSVVideoEncoderPrivate())
{
	libobsqsv11::init(L"obs-qsv11.dll");
}

QSVVideoEncoder::~QSVVideoEncoder()
{
	if(m_private->context)
	{
		libobsqsv11::qsv_encoder_close(m_private->context);
		m_private->context = NULL;
	}
	if(m_private->spspps)
	{
		free(m_private->spspps);
		m_private->spspps = NULL;
		m_private->spsppsLen = 0;
	}
	delete m_private;
	m_private = NULL;
}

bool QSVVideoEncoder::getValidPixelFormat( int* pixfmt, int& count )
{
	if(count<2)	return false;
	pixfmt[0] = (int)zMedia::PIXFMT_E_I420;
	pixfmt[1] = (int)zMedia::PIXFMT_E_NV12;
	count = 2;
	return true;
}

bool QSVVideoEncoder::init()
{
	if(m_private->context!=NULL)
		return false;
	m_private->context = libobsqsv11::qsv_encoder_open(&m_private->params);
	if(!m_private->context)
	{
		//fixme log
		return false;
	}
	//get sps/pps
	loadHeaders();
 	libobsqsv11::qsv_encoder_version(&m_private->verMajor, &m_private->verMinor);
	//fixme 当版本号小于等于1.6时需要自己计算pts与dts
	return true;
}

int QSVVideoEncoder::loadHeaders()
{
	uint8_t *pSPS = NULL, *pPPS = NULL;
	uint16_t nSPS = 0, nPPS = 0;
	int result = libobsqsv11::qsv_encoder_headers(m_private->context, &pSPS, &pPPS, &nSPS, &nPPS);
	uint32_t total = nSPS + nPPS;
	uint8_t* buffer = (uint8_t*)malloc(total*sizeof(uint8_t));
	if(buffer==NULL)
		return -1;
	memcpy(buffer, pSPS, nSPS);
	memcpy(buffer+nSPS, pPPS, nPPS);
	m_private->spspps = buffer;
	m_private->spsppsLen = total;
	// Not sure if SEI is needed.
	// Just filling in empty meaningless SEI message.
	// Seems to work fine.
	// DARRAY(uint8_t) sei;
	return 0;
}

bool QSVVideoEncoder::setConfig( const VideoEncoderConfig& vCfg )
{
	m_private->params.nWidth = vCfg.width;
	m_private->params.nHeight = vCfg.height;
	//choose between BALANCED, Qualify, Speed
	m_private->params.nTargetUsage = MFX_TARGETUSAGE_BALANCED;
	//choose between baseline, main, high
	m_private->params.nCodecProfile = MFX_PROFILE_AVC_BASELINE;
	//fps
	m_private->params.nFpsNum = vCfg.fps;
	m_private->params.nFpsDen = 1;
	//关键帧间隔
	m_private->params.nKeyIntSec = vCfg.gop / vCfg.fps;
	m_private->params.nbFrames = vCfg.numBFrame;
	m_private->params.nTargetBitRate = vCfg.avgBitrate / 1000;
	m_private->params.nMaxBitRate = vCfg.maxBitrate / 1000;
	m_private->params.nAsyncDepth = 4;
	m_private->params.nAccuracy = 1000;
	switch(vCfg.rcMode)
	{
	case VideoEncoderConfig::RC_CBR:
		m_private->params.nRateControl = MFX_RATECONTROL_CBR;
		break;
	case VideoEncoderConfig::RC_VBR:
		m_private->params.nRateControl = MFX_RATECONTROL_VBR;
		break;
	case VideoEncoderConfig::RC_CQP:
		m_private->params.nRateControl = MFX_RATECONTROL_CQP;
		break;
	case VideoEncoderConfig::RC_VBR_MINQP:
		m_private->params.nRateControl = MFX_RATECONTROL_QVBR;
		break;
	case VideoEncoderConfig::RC_2PASS_IMG:
		break;
	case VideoEncoderConfig::RC_2PASS:
		break;
	case VideoEncoderConfig::RC_2PASS_VBR:
		break;
	default:
		//default is CBR
		m_private->params.nRateControl = MFX_RATECONTROL_CBR;
		break;
	}
	return true;
}

/* NOTE: I noticed that FFmpeg does some unusual special handling of certain
 * scenarios that I was unaware of, so instead of just searching for {0, 0, 1}
 * we'll just use the code from FFmpeg - http://www.ffmpeg.org/ */
static const uint8_t *ff_avc_find_startcode_internal(const uint8_t *p,
		const uint8_t *end)
{
	const uint8_t *a = p + 4 - ((intptr_t)p & 3);

	for (end -= 3; p < a && p < end; p++) {
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	for (end -= 3; p < end; p += 4) {
		uint32_t x = *(const uint32_t*)p;

		if ((x - 0x01010101) & (~x) & 0x80808080) {
			if (p[1] == 0) {
				if (p[0] == 0 && p[2] == 1)
					return p;
				if (p[2] == 0 && p[3] == 1)
					return p+1;
			}

			if (p[3] == 0) {
				if (p[2] == 0 && p[4] == 1)
					return p+2;
				if (p[4] == 0 && p[5] == 1)
					return p+3;
			}
		}
	}

	for (end += 3; p < end; p++) {
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	return end + 3;
}

const uint8_t *obs_avc_find_startcode(const uint8_t *p, const uint8_t *end)
{
	const uint8_t *out= ff_avc_find_startcode_internal(p, end);
	if (p < out && out < end && !out[-1]) out--;
	return out;
}

VideoOutputFrame parse_packet(mfxBitstream *pBS, uint32_t fps_num)
{
	uint8_t *start, *end;
	int type;
	VideoOutputFrame encodedFrame(NULL, 0);

	if (pBS == NULL || pBS->DataLength == 0) 
	{
		return encodedFrame;
	}
	encodedFrame.frameData = (unsigned char*)malloc(pBS->DataLength);
	memcpy(encodedFrame.frameData, (uint8_t*)pBS->Data+pBS->DataOffset, pBS->DataLength);
	encodedFrame.frameSize = pBS->DataLength;
	mfxU64 tmpTs = pBS->TimeStamp * fps_num / 90000;
	encodedFrame.pts = (uint32_t)(tmpTs);
	//encodedFrame.nalRefIdc
	if(pBS->FrameType & MFX_FRAMETYPE_I)
		encodedFrame.frameType = FRAME_TYPE_I;
	else if(pBS->FrameType & MFX_FRAMETYPE_P)
		encodedFrame.frameType = FRAME_TYPE_P;
	else if(pBS->FrameType & MFX_FRAMETYPE_B)
		encodedFrame.frameType = FRAME_TYPE_B;
	else if(pBS->FrameType & MFX_FRAMETYPE_IDR)
		encodedFrame.frameType = FRAME_TYPE_IDR;

	// fixme 当Intel sdk版本号小于1.6时需要手动计算dts
	encodedFrame.dts = (uint32_t)(pBS->DecodeTimeStamp * fps_num / 90000);
	encodedFrame.nalType ;
	encodedFrame.nalRefIdc ;

	start = encodedFrame.frameData;
	end = start + pBS->DataLength;
	start = (uint8_t*)obs_avc_find_startcode(start, end);
	while (true) {
		while (start < end && !*(start++));

		if (start == end)
			break;

		type = start[0] & 0x1F;
		if (type == OBS_NAL_SLICE_IDR || type == OBS_NAL_SLICE) {
			uint8_t prev_type = (start[0] >> 5) & 0x3;
			start[0] &= ~(3 << 5);

			if (pBS->FrameType & MFX_FRAMETYPE_I)
				start[0] |= OBS_NAL_PRIORITY_HIGHEST << 5;
			else if (pBS->FrameType & MFX_FRAMETYPE_P)
				start[0] |= OBS_NAL_PRIORITY_HIGH << 5;
			else
				start[0] |= prev_type << 5;
		}

		start = (uint8_t*)obs_avc_find_startcode(start, end);
	}
	return encodedFrame;
}

bool QSVVideoEncoder::encodeFrame( zMedia::VideoData::SPtr vframe )
{
	zMedia::PictureRaw::SPtr picRaw = vframe ? vframe->getRawData() : zMedia::PictureRaw::SPtr();
	if(!picRaw || picRaw->format().ePixfmt!=zMedia::PIXFMT_E_NV12)
		return false;
	mfxBitstream *pBS = NULL;
	int ret;

	// FIXME: remove null check from the top of this function
	// if we actually do expect null frames to complete output.
	mfxU64 qsvPTS = 0;
	if (vframe)
	{
		mfxU64 tmp = picRaw->getTimestamp();
		qsvPTS = tmp * 90000 / m_private->params.nFpsNum;
		uint32_t y_stride = picRaw->format().y_stride;
		uint32_t uv_stride = picRaw->format().u_stride;
		uint8_t* y_data = (uint8_t*)picRaw->y(); 
		uint8_t* uv_data = (uint8_t*)picRaw->uv();
		ret = libobsqsv11::qsv_encoder_encode(
		m_private->context,
		qsvPTS,
		y_data, uv_data, 
		y_stride, uv_stride,
		&pBS);
	}
	else
	{
		qsvPTS = m_private->pts;
		ret = libobsqsv11::qsv_encoder_encode(
								m_private->context,
								qsvPTS,
								NULL, NULL, 0, 0, &pBS);
	}
	if (ret < 0) {
		//fixme log failed.
		//warn("encode failed");
		return false;
	}
	if(NULL==pBS)
	{
		// 送入编码器成功，但是没有输出的数据
		return true;
	}
	VideoOutputFrame encodedFrame = parse_packet(pBS, m_private->params.nFpsNum);
	pBS->DataLength = 0;
	if(encodedFrame.frameData==NULL || 0>=encodedFrame.frameSize)
		return false;
	m_private->encodedFrames.push_back(encodedFrame);
	// 打印帧的时间戳用于调试
// 	char dbgmsg[256] = {0};
// 	sprintf_s(dbgmsg, sizeof(dbgmsg), "QSVVideoEncoder::encodeFrame vframe-ts=%u input-ts=%llu output-ts=%u\n",
// 		vframe->getTimestamp(), qsvPTS, encodedFrame.pts);
// 	OutputDebugStringA(dbgmsg);
	return true;
}

bool QSVVideoEncoder::getEncFrame( VideoOutputFrame& outFrame )
{
	if(m_private->encodedFrames.size()<=0)
		return false;
	VideoOutputFrame tmpFrame = m_private->encodedFrames.front();
	if(outFrame.frameData==NULL || outFrame.frameSize<tmpFrame.frameSize)
		return false;
	m_private->encodedFrames.pop_front();
	memcpy(outFrame.frameData, tmpFrame.frameData, tmpFrame.frameSize);
	outFrame.frameSize = tmpFrame.frameSize;
	outFrame.dts = tmpFrame.dts;
	outFrame.pts = tmpFrame.pts;
	outFrame.frameType = tmpFrame.frameType;
	outFrame.nalRefIdc = tmpFrame.nalRefIdc;
	outFrame.nalType = tmpFrame.nalType;
	free(tmpFrame.frameData);
	return true;
}
