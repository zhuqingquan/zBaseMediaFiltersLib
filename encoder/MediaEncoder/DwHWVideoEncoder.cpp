#include "DwHWVideoEncoder.h"
#include "CriticalSection.h"
#include "MediaSDKEncoder/libhwcodec.h"
#include "Media.h"

using namespace Media;

typedef DWCODEC_RETCODE (* Func_DwCreateVideoEncoder)(VIDEO_CODEC_PROVIDER codec, DwVideoEncoder ** ppVideoEncoder);
typedef void (* Func_DwReleaseVideoEncoder)(DwVideoEncoder * ppVideoEncoder);

static HMODULE g_dll_module = NULL;
Func_DwCreateVideoEncoder g_pCreateFunc = NULL;
Func_DwReleaseVideoEncoder g_pReleaseFunc = NULL;
static CCriticalLock g_module_mutex;

bool loadLibDll(TCHAR* dllPathName)
{
	CAutoLock lock(g_module_mutex);
	if(g_dll_module)	return true;
	g_dll_module = ::LoadLibraryEx(dllPathName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if(NULL==g_dll_module)	return false;
	g_pCreateFunc = (Func_DwCreateVideoEncoder)::GetProcAddress(g_dll_module, "DwCreateVideoEncoder");
	g_pReleaseFunc = (Func_DwReleaseVideoEncoder)::GetProcAddress(g_dll_module, "DwReleaseVideoEncoder");
	return g_dll_module!=NULL;
}

bool freeLibDll()
{
	CAutoLock lock(g_module_mutex);
	if(!g_dll_module)	return false;
	g_pCreateFunc = NULL;
	g_pReleaseFunc = NULL;
	FreeLibrary(g_dll_module);
	g_dll_module = NULL;
	return true;
}

Media::DwHWVideoEncoder::DwHWVideoEncoder(Media::VIDEO_CODEC_IMPL codecType)
: m_encoderImpl(NULL)
{

	//如果未调用loadLibDll则直接不创建任何资源
	if(g_dll_module==NULL || g_pCreateFunc==NULL || g_pReleaseFunc==NULL)
		return;
	DwVideoEncoder* videoEncoder = NULL;
	//HW_CODEC_NVIDIA_NVENC
	//SW_CODEC_X264
	//HW_CODEC_INTEL_QUICKSYNC
	DWCODEC_RETCODE ret = DW_CODEC_ERR_UNKNOWN;
	switch(codecType)
	{
	case Media::HW_CODEC_NVIDIA_NVENC:
		ret = g_pCreateFunc(::HW_CODEC_NVIDIA_NVENC, &videoEncoder);
		break;
	case Media::HW_CODEC_INTEL_QUICKSYNC:
		ret = g_pCreateFunc(::HW_CODEC_INTEL_QUICKSYNC, &videoEncoder);
		break;
	case Media::HW_CODEC_NVIDIA_NVENC_HEVC:
		ret = g_pCreateFunc(::HW_CODEC_NVIDIA_NVENC_HEVC, &videoEncoder);
		break;
	case Media::HW_CODEC_INTEL_QUICKSYNC_HEVC:
		ret = g_pCreateFunc(::HW_CODEC_INTEL_QUICKSYNC_HEVC, &videoEncoder);
		break;
	default:
		break;
	}
	if(DW_CODEC_OK!=ret)
	{
		printf("创建解码器失败 [ret=%d]\n", ret);
		return;
	}
	m_encoderImpl = videoEncoder;
}

Media::DwHWVideoEncoder::~DwHWVideoEncoder()
{
	if(m_encoderImpl)
	{
		m_encoderImpl->endEncode();
		g_pReleaseFunc(m_encoderImpl);
		m_encoderImpl = NULL;
	}
}

bool Media::DwHWVideoEncoder::init()
{
	if(NULL==m_encoderImpl)	return false;
	bool ret = m_encoderImpl->init();
	if(!ret)
	{
		const char* errmsg = m_encoderImpl->getLastErrString();
		OutputDebugStringA(errmsg);
	}
	return ret;
}

bool Media::DwHWVideoEncoder::setConfig(const VideoEncoderConfig& vCfg)
{
	if(NULL==m_encoderImpl)	return false;
	DwVideoEncoderConfig dwVCfg;
	dwVCfg.avgBitrate = vCfg.avgBitrate;
	dwVCfg.maxBitrate = vCfg.maxBitrate;
	dwVCfg.minBitrate = vCfg.minBitrate;
	memcpy(dwVCfg.configString, vCfg.configString, sizeof(dwVCfg.configString));
	dwVCfg.fps = vCfg.fps;
	dwVCfg.gop = vCfg.gop;
	dwVCfg.width = vCfg.width;
	dwVCfg.height = vCfg.height;
	dwVCfg.numBFrame = vCfg.numBFrame;
	dwVCfg.outputPkgMode = vCfg.outputPkgMode;
	dwVCfg.qp = vCfg.qp;
	dwVCfg.rcMode = vCfg.rcMode;
	dwVCfg.thread_count = vCfg.thread_count;
	if(!m_encoderImpl->setConfig(dwVCfg))
		return false;
	m_encConfig = vCfg;
	return m_encoderImpl->beginEncode();
}

#include "Picture.h"
#include "Buffer.h"
bool Media::DwHWVideoEncoder::encodeFrame(boost::shared_ptr<VideoFrame> vframe)
{
	if(NULL==m_encoderImpl)	return false;
	if(NULL==vframe)	return false;
	boost::shared_ptr<VideoFrame> tmpFrame = vframe;
	if(Video::COLOR_E_I420!=vframe->getPixfmt())
	{
		tmpFrame = convertPixfmt(vframe, Video::COLOR_E_I420);
	}
// 	static unsigned char* pTmpBuf = NULL;
// 	if(pTmpBuf==NULL)
// 	{
// 		pTmpBuf = (unsigned char*)malloc(tmpFrame->getDataLen());
// 	}
// 	memcpy(pTmpBuf, tmpFrame->getData(), tmpFrame->getDataLen());
// 	Video::PICTURE* pic = new Video::PICTURE(Video::PICTURE_HEADER(1920, 1080, Video::COLOR_E_I420, 1920, 960));
// 	Video::CBuffer* pBuf = new Video::CBuffer();
// 	int picSize = GetPictureSize(&pic->header);
// 	pBuf->checkBufferSize(picSize);
// 	pic->pBits = pBuf->getPtr();
// 	pBuf->setDataSize(picSize);
// 	boost::shared_ptr<VideoFrame> vtestframe = boost::make_shared<VideoFrame>(pic, pBuf);
// 	memcpy(vtestframe->getData(), tmpFrame->getData(), tmpFrame->getDataLen());
// 	tmpFrame = vtestframe;

	DwVideoRawFrame srcDwFrame;
	srcDwFrame.width = tmpFrame->getWidth();
	srcDwFrame.height = tmpFrame->getHeight();
	if(srcDwFrame.width!=m_encConfig.width || srcDwFrame.height!=m_encConfig.height)
	{
		return false;
	}
	srcDwFrame.format = (FOURCC_ColorFormat)0x30323449/*FOURCC_I420*/;
	unsigned char* pBufStart = tmpFrame->getData();//pTmpBuf;
	int ystride = tmpFrame->getPicInfo()->header.y_stride;
	int uvstride = tmpFrame->getPicInfo()->header.uv_stride;
	srcDwFrame.planeptrs[0].ptr = (uint8_t*)pBufStart;
	srcDwFrame.planeptrs[1].ptr = (uint8_t*)pBufStart + ystride * srcDwFrame.height;
	srcDwFrame.planeptrs[2].ptr = (uint8_t*)pBufStart + ystride * srcDwFrame.height + uvstride * ((srcDwFrame.height + 1) >> 1);
	srcDwFrame.strides[0] = ystride;
	srcDwFrame.strides[1] = uvstride;
	srcDwFrame.strides[2] = uvstride;
	//static int ts = 0;
	srcDwFrame.timestamp = tmpFrame->getTimestamp();//ts;
	//ts += 40;
	if(!m_encoderImpl->addRawFrame(srcDwFrame))
		return false;
	return true;;
}

bool Media::DwHWVideoEncoder::getEncFrame(VideoOutputFrame& outFrame)
{
	if(NULL==m_encoderImpl)	return false;
	DwVideoOutputFrame outDwFrame;
	if(m_encoderImpl->getEncodedFrame(outDwFrame))
	{
		if(outDwFrame.frameSize<=0 || outDwFrame.frameData.ptr==NULL)
			return false;
		if(outFrame.frameData==NULL || outFrame.frameSize<=0)
			return false;
		if(outFrame.frameSize<outDwFrame.frameSize)
			return false;
		outFrame.nalRefIdc = outDwFrame.nalRefIdc;
		outFrame.nalType = outDwFrame.nalType;
		outFrame.pts = outDwFrame.pts;
		outFrame.dts = outFrame.pts;//outDwFrame.dts;
		switch(outDwFrame.frameType)
		{
		case IDR_Frame:
			outFrame.frameType = Media::FRAME_TYPE_IDR;
			break;
		case I_Frame:
			//len_pps_sps = videoEncoder->getSps(pps_sps);
			//len_pps_sps = videoEncoder->getPps(pps_sps);
			//len_pps_sps = videoEncoder->getVps(pps_sps);
			//outputFile.write((char*)outFrame.frameData.ptr, outFrame.frameSize);
			outFrame.frameType = Media::FRAME_TYPE_I;
			break;
		case P_Frame:
			//outputFile.write((char*)outFrame.frameData.ptr, outFrame.frameSize);
			outFrame.frameType = Media::FRAME_TYPE_P;
			break;
		case B_Frame:
			//outputFile.write((char*)outFrame.frameData.ptr, outFrame.frameSize);
			outFrame.frameType = Media::FRAME_TYPE_B;
			break;
		default:
			break;
		}
		memcpy(outFrame.frameData, outDwFrame.frameData.ptr, outDwFrame.frameSize);
		outFrame.frameSize = outDwFrame.frameSize;
		m_encoderImpl->freeBuffer(outDwFrame);
		return true;
	}
	return false;
}

bool Media::DwHWVideoEncoder::getValidPixelFormat( int* pixfmt, int& count )
{
	if(count<2)	return false;
	pixfmt[0] = (int)Video::COLOR_E_I420;
	pixfmt[1] = (int)Video::COLOR_E_NV12;
	count = 2;
	return true;
}
