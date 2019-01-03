#include "NVSDKVideoEncoder.h"
#include "AudioData.h"
#include "NvHWEncoder.h"
#include <list>
#include <dxgi.h>
#include "d3d10_1.h"
#include "d3d11.h"

using namespace Media;

#define BITSTREAM_BUFFER_SIZE 2 * 1024 * 1024
#define MAX_ENCODE_QUEUE 32
#define FRAME_QUEUE 240
#define NUM_OF_MVHINTS_PER_BLOCK8x8   4
#define NUM_OF_MVHINTS_PER_BLOCK8x16  2
#define NUM_OF_MVHINTS_PER_BLOCK16x8  2
#define NUM_OF_MVHINTS_PER_BLOCK16x16 1

typedef enum 
{
	NV_ENC_DX9 = 0,
	NV_ENC_DX11 = 1,
	NV_ENC_CUDA = 2,
	NV_ENC_DX10 = 3,
} NvEncodeDeviceType;

template<class T>
class CNvQueue {
	T** m_pBuffer;
	unsigned int m_uSize;
	unsigned int m_uPendingCount;
	unsigned int m_uAvailableIdx;
	unsigned int m_uPendingndex;
public:
	CNvQueue(): m_pBuffer(NULL), m_uSize(0), m_uPendingCount(0), m_uAvailableIdx(0),
		m_uPendingndex(0)
	{
	}

	~CNvQueue()
	{
		delete[] m_pBuffer;
	}

	bool Initialize(T *pItems, unsigned int uSize)
	{
		m_uSize = uSize;
		m_uPendingCount = 0;
		m_uAvailableIdx = 0;
		m_uPendingndex = 0;
		m_pBuffer = new T *[m_uSize];
		for (unsigned int i = 0; i < m_uSize; i++)
		{
			m_pBuffer[i] = &pItems[i];
		}
		return true;
	}


	T * GetAvailable()
	{
		T *pItem = NULL;
		if (m_uPendingCount == m_uSize)
		{
			return NULL;
		}
		pItem = m_pBuffer[m_uAvailableIdx];
		m_uAvailableIdx = (m_uAvailableIdx+1)%m_uSize;
		m_uPendingCount += 1;
		return pItem;
	}

	T* GetPending()
	{
		if (m_uPendingCount == 0) 
		{
			return NULL;
		}

		T *pItem = m_pBuffer[m_uPendingndex];
		m_uPendingndex = (m_uPendingndex+1)%m_uSize;
		m_uPendingCount -= 1;
		return pItem;
	}
};

typedef struct _EncodeFrameConfig
{
	uint8_t  *yuv[3];
	uint32_t stride[3];
	uint32_t width;
	uint32_t height;
	int8_t *qpDeltaMapArray;
	uint32_t qpDeltaMapArraySize;
	NVENC_EXTERNAL_ME_HINT *meExternalHints;
	NVENC_EXTERNAL_ME_HINT_COUNTS_PER_BLOCKTYPE meHintCountsPerBlock[1];
}EncodeFrameConfig;

struct NVSDKVideoEncoderPrivate
{
	zMedia::E_PIXFMT format;
	EncodeConfig encodeConfig;		// 设置的编码参数属性
	void* m_pDevice;
#if defined(NV_WINDOWS)
	IDirect3D9* m_pD3D;
#endif
	CNvHWEncoder* m_pNvHWEncoder;
	uint32_t m_uEncodeBufferCount;
	uint32_t m_uPicStruct;
	EncodeBuffer                                        m_stEncodeBuffer[MAX_ENCODE_QUEUE];
	CNvQueue<EncodeBuffer>                              m_EncodeBufferQueue;
	EncodeOutputBuffer                                  m_stEOSOutputBfr; 
	std::list<VideoOutputFrame>							m_encodedFrames;

	NVSDKVideoEncoderPrivate()
		: format(zMedia::PIXFMT_E_NONE)
		, m_pDevice(NULL), m_pNvHWEncoder(NULL)
		, m_uEncodeBufferCount(0), m_uPicStruct(0)
#if defined(NV_WINDOWS)
		, m_pD3D(NULL)
#endif
	{
		memset(&encodeConfig, 0, sizeof(EncodeConfig));
		encodeConfig.endFrameIdx = INT_MAX;
		encodeConfig.bitrate = 5000000;
		encodeConfig.rcMode = NV_ENC_PARAMS_RC_CONSTQP;
		encodeConfig.gopLength = NVENC_INFINITE_GOPLENGTH;
		encodeConfig.deviceType = NV_ENC_CUDA;//NV_ENC_DX11;
		encodeConfig.codec = NV_ENC_H264;
		encodeConfig.fps = 30;
		encodeConfig.qp = 28;
		encodeConfig.i_quant_factor = DEFAULT_I_QFACTOR;
		encodeConfig.b_quant_factor = DEFAULT_B_QFACTOR;
		encodeConfig.i_quant_offset = DEFAULT_I_QOFFSET;
		encodeConfig.b_quant_offset = DEFAULT_B_QOFFSET; 
		encodeConfig.presetGUID = NV_ENC_PRESET_DEFAULT_GUID;
		encodeConfig.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
		encodeConfig.inputFormat = NV_ENC_BUFFER_FORMAT_NV12;
	}

	NVENCSTATUS InitD3D9(uint32_t deviceID = 0);
	NVENCSTATUS InitD3D11(uint32_t deviceID = 0);
	NVENCSTATUS InitD3D10(uint32_t deviceID = 0);
	NVENCSTATUS InitCuda(uint32_t deviceID = 0);
	NVENCSTATUS AllocateIOBuffers(uint32_t uInputWidth, uint32_t uInputHeight, NV_ENC_BUFFER_FORMAT inputFormat);
	NVENCSTATUS ReleaseIOBuffers();
	NVENCSTATUS Deinitialize(uint32_t devicetype);
	NVENCSTATUS EncodeFrame(EncodeFrameConfig *pEncodeFrame, bool bFlush, uint32_t width, uint32_t height);
	NVENCSTATUS FlushEncoder();
	VideoOutputFrame getEncodedFrame(EncodeBuffer *pEncodeBufer);
};

NVSDKVideoEncoder::NVSDKVideoEncoder()
: m_private(new NVSDKVideoEncoderPrivate)
{
	m_private->m_pNvHWEncoder = new CNvHWEncoder();
}

NVSDKVideoEncoder::~NVSDKVideoEncoder()
{
	deinitEncodeObj();

	delete m_private->m_pNvHWEncoder;
	m_private->m_pNvHWEncoder = NULL;

	delete m_private;
	m_private = NULL;
}

bool NVSDKVideoEncoder::init()
{
	NVENCSTATUS nvStatus;
	switch (m_private->encodeConfig.deviceType)
	{
#if defined(NV_WINDOWS)
case NV_ENC_DX9:
	m_private->InitD3D9(m_private->encodeConfig.deviceID);
	break;

case NV_ENC_DX10:
	m_private->InitD3D10(m_private->encodeConfig.deviceID);
	break;

case NV_ENC_DX11:
	m_private->InitD3D11(m_private->encodeConfig.deviceID);
	break;
#endif
case NV_ENC_CUDA:
	m_private->InitCuda(m_private->encodeConfig.deviceID);
	break;
	}
	if (m_private->encodeConfig.deviceType != NV_ENC_CUDA)
		nvStatus = m_private->m_pNvHWEncoder->Initialize(m_private->m_pDevice, NV_ENC_DEVICE_TYPE_DIRECTX);
	else
		nvStatus = m_private->m_pNvHWEncoder->Initialize(m_private->m_pDevice, NV_ENC_DEVICE_TYPE_CUDA);

	if (nvStatus != NV_ENC_SUCCESS)
		return false;
	m_private->encodeConfig.presetGUID = m_private->m_pNvHWEncoder->GetPresetGUID(m_private->encodeConfig.encoderPreset, m_private->encodeConfig.codec);

	nvStatus = m_private->m_pNvHWEncoder->CreateEncoder(&m_private->encodeConfig);
	if (nvStatus != NV_ENC_SUCCESS)
		return false;
	m_private->encodeConfig.maxWidth = m_private->encodeConfig.maxWidth ? m_private->encodeConfig.maxWidth : m_private->encodeConfig.width;
	m_private->encodeConfig.maxHeight = m_private->encodeConfig.maxHeight ? m_private->encodeConfig.maxHeight : m_private->encodeConfig.height;

/*	m_stEncoderInput.enableAsyncMode = encodeConfig.enableAsyncMode;

	if (encodeConfig.enableExternalMEHint && (m_stEncoderInput.enableMEOnly || 
		encodeConfig.codec != NV_ENC_H264 || encodeConfig.numB > 0))
	{
		printf("Application supports external hint only for H264 encoding for P frame \n");
		return 1;
	}
*/
	// 计算编码所需的缓冲Buffer个数
	if (m_private->encodeConfig.numB > 0)
	{
		m_private->m_uEncodeBufferCount = m_private->encodeConfig.numB + 4; // min buffers is numb + 1 + 3 pipelining
	}
	else
	{
		int numMBs = ((m_private->encodeConfig.maxHeight + 15) >> 4) * ((m_private->encodeConfig.maxWidth + 15) >> 4);
		int NumIOBuffers;
		if (numMBs >= 32768) //4kx2k
			NumIOBuffers = MAX_ENCODE_QUEUE / 8;
		else if (numMBs >= 16384) // 2kx2k
			NumIOBuffers = MAX_ENCODE_QUEUE / 4;
		else if (numMBs >= 8160) // 1920x1080
			NumIOBuffers = MAX_ENCODE_QUEUE / 2;
		else
			NumIOBuffers = MAX_ENCODE_QUEUE;
		m_private->m_uEncodeBufferCount = NumIOBuffers;
	}
	m_private->m_uPicStruct = m_private->encodeConfig.pictureStruct;
	nvStatus = m_private->AllocateIOBuffers(m_private->encodeConfig.width, m_private->encodeConfig.height, m_private->encodeConfig.inputFormat);
	if (nvStatus != NV_ENC_SUCCESS)
		return false;
	return true;
}

bool isValidPixelFormat(NVSDKVideoEncoder* encoder, int pixfmt)
{
	int fmtArray[64] = {0};
	int count = 64;
	if(!encoder->getValidPixelFormat(fmtArray, count))
		return false;
	for (int i=0; i<count; i++)
	{
		if(pixfmt==fmtArray[i])
			return true;
	}
	return false;
}

bool NVSDKVideoEncoder::setConfig( const VideoEncoderConfig& vCfg )
{
	if(!isValidPixelFormat(this, vCfg.pixfmt))
		return false;
	m_private->format = (zMedia::E_PIXFMT)vCfg.pixfmt;
	switch(m_private->format)
	{
	case zMedia::PIXFMT_E_I420:
		//当外部输入I420，则将I420拷贝到YV12格式的Surface中
		m_private->encodeConfig.inputFormat = NV_ENC_BUFFER_FORMAT_YV12;
		break;
	case zMedia::PIXFMT_E_NV12:
		m_private->encodeConfig.inputFormat = NV_ENC_BUFFER_FORMAT_NV12;
		break;
	case zMedia::PIXFMT_E_YV12:
		m_private->encodeConfig.inputFormat = NV_ENC_BUFFER_FORMAT_YV12;
		break;
	default:
		return false;
	}
	m_private->encodeConfig.codec = NV_ENC_H264;	//只支持H264
	m_private->encodeConfig.width = vCfg.width;
	m_private->encodeConfig.height = vCfg.height;
	m_private->encodeConfig.fps = vCfg.fps;
	m_private->encodeConfig.gopLength = vCfg.gop;
	m_private->encodeConfig.numB = vCfg.numBFrame;
	m_private->encodeConfig.qp = vCfg.qp;
	m_private->encodeConfig.bitrate = vCfg.avgBitrate;
	switch(vCfg.rcMode)
	{
	case VideoEncoderConfig::RC_CBR:
		m_private->encodeConfig.rcMode = 2;
		break;
	case VideoEncoderConfig::RC_VBR:
		m_private->encodeConfig.rcMode = 1;
		break;
	case VideoEncoderConfig::RC_CQP:
		m_private->encodeConfig.rcMode = 0;
		break;
	case VideoEncoderConfig::RC_VBR_MINQP:
		m_private->encodeConfig.rcMode = 16;
		break;
	default:
		return false;
	}
	
	return true;
}

bool Media::NVSDKVideoEncoder::getValidPixelFormat( int* pixfmt, int& count )
{
	if(count<3)	return false;
	pixfmt[0] = (int)zMedia::PIXFMT_E_I420;
	pixfmt[1] = (int)zMedia::PIXFMT_E_YV12;
	pixfmt[2] = (int)zMedia::PIXFMT_E_NV12;
	count = 3;
	return true;
}

int Media::NVSDKVideoEncoder::initEncodeObj()
{
	return -1;
}

int Media::NVSDKVideoEncoder::deinitEncodeObj()
{
	m_private->Deinitialize(m_private->encodeConfig.deviceType);
	return 0;
}

bool Media::NVSDKVideoEncoder::encodeFrame(zMedia::VideoData::SPtr vframe )
{
	NVENCSTATUS nvStatus;
	if(vframe==NULL)
	{
		nvStatus = m_private->EncodeFrame(NULL, true, m_private->encodeConfig.width, m_private->encodeConfig.height);
		if (nvStatus != NV_ENC_SUCCESS)
		{
			return false;
		}
		// fixme 获取缓冲的编码成功的h264数据

		return true;
	}
	zMedia::PictureRaw::SPtr picRaw = vframe->getRawData();
	if(!isValidPixelFormat(this, picRaw->format().ePixfmt))
		return false;
	if(picRaw->format().w!=m_private->encodeConfig.width || picRaw->format().h!=m_private->encodeConfig.height)
	{
		// 这里没有scaler
		return false;
	}
	EncodeFrameConfig stEncodeFrame;
	memset(&stEncodeFrame, 0, sizeof(stEncodeFrame));
	//Video::PICTURE* picInfo = vframe->getPicInfo();
	unsigned char* startPos = picRaw->data();
	switch(picRaw->format().ePixfmt)
	{
	case zMedia::PIXFMT_E_I420:
		stEncodeFrame.yuv[0] = (uint8_t*)picRaw->y();// startPos;
		stEncodeFrame.yuv[1] = (uint8_t*)picRaw->u();//startPos + picInfo->header.h * picInfo->header.y_stride;
		stEncodeFrame.yuv[2] = (uint8_t*)picRaw->v();//stEncodeFrame.yuv[1] + (picInfo->header.h >> 1) * picInfo->header.uv_stride;

		stEncodeFrame.stride[0] = picRaw->format().y_stride;//picInfo->header.y_stride;//encodeConfig.width * (encodeConfig.inputFormat == NV_ENC_BUFFER_FORMAT_YUV420_10BIT || encodeConfig.inputFormat == NV_ENC_BUFFER_FORMAT_YUV444_10BIT ? 2 : 1);
		stEncodeFrame.stride[1] = picRaw->format().u_stride;//picInfo->header.uv_stride;//stEncodeFrame.stride[2] = chromaFormatIDC == 3 ? stEncodeFrame.stride[0] : stEncodeFrame.stride[0] / 2;
		stEncodeFrame.stride[2] = picRaw->format().v_stride;//picInfo->header.uv_stride;
		break;
	case zMedia::PIXFMT_E_NV12:
		stEncodeFrame.yuv[0] = (uint8_t*)picRaw->y();//startPos;
		stEncodeFrame.yuv[1] = (uint8_t*)picRaw->uv();//startPos + picInfo->header.h * picInfo->header.y_stride;
		stEncodeFrame.yuv[2] = NULL;

		stEncodeFrame.stride[0] = picRaw->format().y_stride;//picInfo->header.y_stride;
		stEncodeFrame.stride[1] = picRaw->format().u_stride;//picInfo->header.uv_stride;
		stEncodeFrame.stride[2] = 0;
		break;
	case zMedia::PIXFMT_E_YV12:
		stEncodeFrame.yuv[0] = (uint8_t*)picRaw->y(); //startPos;
		stEncodeFrame.yuv[2] = (uint8_t*)picRaw->u(); //startPos + picInfo->header.h * picInfo->header.y_stride;
		stEncodeFrame.yuv[1] = (uint8_t*)picRaw->v(); //stEncodeFrame.yuv[2] + (picInfo->header.h >> 1) * picInfo->header.uv_stride;

		stEncodeFrame.stride[0] = picRaw->format().y_stride;//picInfo->header.y_stride;//encodeConfig.width * (encodeConfig.inputFormat == NV_ENC_BUFFER_FORMAT_YUV420_10BIT || encodeConfig.inputFormat == NV_ENC_BUFFER_FORMAT_YUV444_10BIT ? 2 : 1);
		stEncodeFrame.stride[1] = picRaw->format().u_stride;//picInfo->header.uv_stride;//stEncodeFrame.stride[2] = chromaFormatIDC == 3 ? stEncodeFrame.stride[0] : stEncodeFrame.stride[0] / 2;
		stEncodeFrame.stride[2] = picRaw->format().v_stride;//picInfo->header.uv_stride;
		break;
	default:
		return false;
	}
/*	stEncodeFrame.yuv[0] = yuv[0];
	stEncodeFrame.yuv[1] = yuv[1];
	stEncodeFrame.yuv[2] = yuv[2];

	stEncodeFrame.stride[0] = encodeConfig.width * (encodeConfig.inputFormat == NV_ENC_BUFFER_FORMAT_YUV420_10BIT || encodeConfig.inputFormat == NV_ENC_BUFFER_FORMAT_YUV444_10BIT ? 2 : 1);
	stEncodeFrame.stride[1] = stEncodeFrame.stride[2] = chromaFormatIDC == 3 ? stEncodeFrame.stride[0] : stEncodeFrame.stride[0] / 2;*/
	stEncodeFrame.width = m_private->encodeConfig.width;
	stEncodeFrame.height = m_private->encodeConfig.height;
	stEncodeFrame.qpDeltaMapArray = NULL;//qpDeltaMapArray;
	stEncodeFrame.qpDeltaMapArraySize = 0;//qpDeltaMapArraySize;
	nvStatus = m_private->EncodeFrame(&stEncodeFrame, false, m_private->encodeConfig.width, m_private->encodeConfig.height);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		return false;
	}
	// EncodeFrame中已经实现了获取缓冲的编码成功的h264数据

	return true;
}

bool Media::NVSDKVideoEncoder::getEncFrame( VideoOutputFrame& outFrame )
{
	if(m_private->m_encodedFrames.size()<=0)
		return false;
	VideoOutputFrame tmpFrame = m_private->m_encodedFrames.front();
	if(outFrame.frameData==NULL || outFrame.frameSize<tmpFrame.frameSize)
		return false;
	m_private->m_encodedFrames.pop_front();
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

////////////// NVSDKVideoEncoderPrivate /////////////////////////////
void copyNV12( unsigned char* src_luma, unsigned char* src_chroma,
			  int src_yStride, int src_uvStride,
			  unsigned char* dst_luma, unsigned char* dst_chroma,
			  int dst_yStride, int dst_uvStride, int width, int height)
{
	int y;
	int x;
	if (src_yStride == 0)
		src_yStride = width;
	if (dst_yStride == 0)
		dst_yStride = width;
	if (src_uvStride == 0)
		src_uvStride = width >> 1;
	if (dst_uvStride == 0)
		dst_uvStride = width >> 1;

	for ( y = 0 ; y < height ; y++)
	{
		memcpy( dst_luma + (dst_yStride*y), src_luma + (src_yStride*y) , width );
	}

	for ( y = 0 ; y < height/2 ; y++)
	{
		memcpy( dst_chroma + (dst_uvStride*y), src_chroma + (src_uvStride*y) , width );
	}
}

void convertYUVpitchtoNV12( unsigned char *yuv_luma, unsigned char *yuv_cb, unsigned char *yuv_cr,
						   unsigned char *nv12_luma, unsigned char *nv12_chroma,
						   int width, int height , int srcStride, int dstStride)
{
	int y;
	int x;
	if (srcStride == 0)
		srcStride = width;
	if (dstStride == 0)
		dstStride = width;

	for ( y = 0 ; y < height ; y++)
	{
		memcpy( nv12_luma + (dstStride*y), yuv_luma + (srcStride*y) , width );
	}

	for ( y = 0 ; y < height/2 ; y++)
	{
		for ( x= 0 ; x < width; x=x+2)
		{
			nv12_chroma[(y*dstStride) + x] =    yuv_cb[((srcStride/2)*y) + (x >>1)];
			nv12_chroma[(y*dstStride) +(x+1)] = yuv_cr[((srcStride/2)*y) + (x >>1)];
		}
	}
}

void convertYUV10pitchtoP010PL(unsigned short *yuv_luma, unsigned short *yuv_cb, unsigned short *yuv_cr,
							   unsigned short *nv12_luma, unsigned short *nv12_chroma, int width, int height, int srcStride, int dstStride)
{
	int x, y;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			nv12_luma[(y*dstStride / 2) + x] = yuv_luma[(srcStride*y) + x] << 6;
		}
	}

	for (y = 0; y < height / 2; y++)
	{
		for (x = 0; x < width; x = x + 2)
		{
			nv12_chroma[(y*dstStride / 2) + x] = yuv_cb[((srcStride / 2)*y) + (x >> 1)] << 6;
			nv12_chroma[(y*dstStride / 2) + (x + 1)] = yuv_cr[((srcStride / 2)*y) + (x >> 1)] << 6;
		}
	}
}

void convertYUVpitchtoYUV444(unsigned char *yuv_luma, unsigned char *yuv_cb, unsigned char *yuv_cr,
							 unsigned char *surf_luma, unsigned char *surf_cb, unsigned char *surf_cr, int width, int height, int srcStride, int dstStride)
{
	int h;

	for (h = 0; h < height; h++)
	{
		memcpy(surf_luma + dstStride * h, yuv_luma + srcStride * h, width);
		memcpy(surf_cb + dstStride * h, yuv_cb + srcStride * h, width);
		memcpy(surf_cr + dstStride * h, yuv_cr + srcStride * h, width);
	}
}

void convertYUV10pitchtoYUV444(unsigned short *yuv_luma, unsigned short *yuv_cb, unsigned short *yuv_cr,
							   unsigned short *surf_luma, unsigned short *surf_cb, unsigned short *surf_cr,
							   int width, int height, int srcStride, int dstStride)
{
	int x, y;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			surf_luma[(y*dstStride / 2) + x] = yuv_luma[(srcStride*y) + x] << 6;
			surf_cb[(y*dstStride / 2) + x] = yuv_cb[(srcStride*y) + x] << 6;
			surf_cr[(y*dstStride / 2) + x] = yuv_cr[(srcStride*y) + x] << 6;
		}
	}
}

NVENCSTATUS NVSDKVideoEncoderPrivate::InitCuda(uint32_t deviceID)
{
	CUresult cuResult;
	CUdevice device;
	CUcontext cuContextCurr;
	int  deviceCount = 0;
	int  SMminor = 0, SMmajor = 0;

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
	typedef HMODULE CUDADRIVER;
#else
	typedef void *CUDADRIVER;
#endif
	CUDADRIVER hHandleDriver = 0;
	cuResult = cuInit(0, __CUDA_API_VERSION, hHandleDriver);
	if (cuResult != CUDA_SUCCESS)
	{
		PRINTERR("cuInit error:0x%x\n", cuResult);
		assert(0);
		return NV_ENC_ERR_NO_ENCODE_DEVICE;
	}

	cuResult = cuDeviceGetCount(&deviceCount);
	if (cuResult != CUDA_SUCCESS)
	{
		PRINTERR("cuDeviceGetCount error:0x%x\n", cuResult);
		assert(0);
		return NV_ENC_ERR_NO_ENCODE_DEVICE;
	}

	// If dev is negative value, we clamp to 0
	if ((int)deviceID < 0)
		deviceID = 0;

	if (deviceID >(unsigned int)deviceCount - 1)
	{
		PRINTERR("Invalid Device Id = %d\n", deviceID);
		return NV_ENC_ERR_INVALID_ENCODERDEVICE;
	}

	cuResult = cuDeviceGet(&device, deviceID);
	if (cuResult != CUDA_SUCCESS)
	{
		PRINTERR("cuDeviceGet error:0x%x\n", cuResult);
		return NV_ENC_ERR_NO_ENCODE_DEVICE;
	}

	char devName[512] = {0};
	cuResult = cuDeviceGetName(devName, sizeof(devName), device);
	if (cuResult != CUDA_SUCCESS)
	{
		PRINTERR("cuDeviceGet error:0x%x\n", cuResult);
		return NV_ENC_ERR_NO_ENCODE_DEVICE;
	}

	cuResult = cuDeviceComputeCapability(&SMmajor, &SMminor, deviceID);
	if (cuResult != CUDA_SUCCESS)
	{
		PRINTERR("cuDeviceComputeCapability error:0x%x\n", cuResult);
		return NV_ENC_ERR_NO_ENCODE_DEVICE;
	}

	if (((SMmajor << 4) + SMminor) < 0x30)
	{
		PRINTERR("GPU %d does not have NVENC capabilities exiting\n", deviceID);
		return NV_ENC_ERR_NO_ENCODE_DEVICE;
	}

	cuResult = cuCtxCreate((CUcontext*)(&m_pDevice), 0, device);
	if (cuResult != CUDA_SUCCESS)
	{
		PRINTERR("cuCtxCreate error:0x%x\n", cuResult);
		assert(0);
		return NV_ENC_ERR_NO_ENCODE_DEVICE;
	}

	cuResult = cuCtxPopCurrent(&cuContextCurr);
	if (cuResult != CUDA_SUCCESS)
	{
		PRINTERR("cuCtxPopCurrent error:0x%x\n", cuResult);
		assert(0);
		return NV_ENC_ERR_NO_ENCODE_DEVICE;
	}
	return NV_ENC_SUCCESS;
}

#if defined(NV_WINDOWS)
NVENCSTATUS NVSDKVideoEncoderPrivate::InitD3D9(uint32_t deviceID)
{
	D3DPRESENT_PARAMETERS d3dpp;
	D3DADAPTER_IDENTIFIER9 adapterId;
	unsigned int iAdapter = NULL; // Our adapter
	HRESULT hr = S_OK;

	m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (m_pD3D == NULL)
	{
		assert(m_pD3D);
		return NV_ENC_ERR_OUT_OF_MEMORY;;
	}

	if (deviceID >= m_pD3D->GetAdapterCount())
	{
		PRINTERR("Invalid Device Id = %d\n. Please use DX10/DX11 to detect headless video devices.\n", deviceID);
		return NV_ENC_ERR_INVALID_ENCODERDEVICE;
	}

	hr = m_pD3D->GetAdapterIdentifier(deviceID, 0, &adapterId);
	if (hr != S_OK)
	{
		PRINTERR("Invalid Device Id = %d\n", deviceID);
		return NV_ENC_ERR_INVALID_ENCODERDEVICE;
	}

	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferWidth = 640;
	d3dpp.BackBufferHeight = 480;
	d3dpp.BackBufferCount = 1;
	d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dpp.Flags = D3DPRESENTFLAG_VIDEO;//D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	DWORD dwBehaviorFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING;

	hr = m_pD3D->CreateDevice(deviceID,
		D3DDEVTYPE_HAL,
		GetDesktopWindow(),
		dwBehaviorFlags,
		&d3dpp,
		(IDirect3DDevice9**)(&m_pDevice));

	if (FAILED(hr))
		return NV_ENC_ERR_OUT_OF_MEMORY;

	return  NV_ENC_SUCCESS;
}

NVENCSTATUS NVSDKVideoEncoderPrivate::InitD3D10(uint32_t deviceID)
{
	HRESULT hr;
	IDXGIFactory * pFactory = NULL;
	IDXGIAdapter * pAdapter;

	//if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory) != S_OK)
	if (CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory) != S_OK)
	{
		return NV_ENC_ERR_GENERIC;
	}

	if (pFactory->EnumAdapters(deviceID, &pAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		hr = D3D10CreateDevice(pAdapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
			D3D10_SDK_VERSION, (ID3D10Device**)(&m_pDevice));
		if (FAILED(hr))
		{
			PRINTERR("Problem while creating %d D3d10 device \n", deviceID);
			return NV_ENC_ERR_OUT_OF_MEMORY;
		}
	}
	else
	{
		PRINTERR("Invalid Device Id = %d\n", deviceID);
		return NV_ENC_ERR_INVALID_ENCODERDEVICE;
	}

	return  NV_ENC_SUCCESS;
}

NVENCSTATUS NVSDKVideoEncoderPrivate::InitD3D11(uint32_t deviceID)
{
	HRESULT hr;
	IDXGIFactory * pFactory = NULL;
	IDXGIAdapter * pAdapter;

	if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory) != S_OK)
	{
		return NV_ENC_ERR_GENERIC;
	}

	if (pFactory->EnumAdapters(deviceID, &pAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC adptDesc;
		pAdapter->GetDesc(&adptDesc);
		hr = D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0,
			NULL, 0, D3D11_SDK_VERSION, (ID3D11Device**)(&m_pDevice), NULL, NULL);
		if (FAILED(hr))
		{
			PRINTERR("Problem while creating %d D3d11 device \n", deviceID);
			return NV_ENC_ERR_OUT_OF_MEMORY;
		}
	}
	else
	{
		PRINTERR("Invalid Device Id = %d\n", deviceID);
		return NV_ENC_ERR_INVALID_ENCODERDEVICE;
	}

	return  NV_ENC_SUCCESS;
}

NVENCSTATUS NVSDKVideoEncoderPrivate::AllocateIOBuffers(uint32_t uInputWidth, uint32_t uInputHeight, NV_ENC_BUFFER_FORMAT inputFormat)
{
	NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

	m_EncodeBufferQueue.Initialize(m_stEncodeBuffer, m_uEncodeBufferCount);
	for (uint32_t i = 0; i < m_uEncodeBufferCount; i++)
	{
		nvStatus = m_pNvHWEncoder->NvEncCreateInputBuffer(uInputWidth, uInputHeight, &m_stEncodeBuffer[i].stInputBfr.hInputSurface, inputFormat);
		if (nvStatus != NV_ENC_SUCCESS)
			return nvStatus;

		m_stEncodeBuffer[i].stInputBfr.bufferFmt = inputFormat;
		m_stEncodeBuffer[i].stInputBfr.dwWidth = uInputWidth;
		m_stEncodeBuffer[i].stInputBfr.dwHeight = uInputHeight;
		nvStatus = m_pNvHWEncoder->NvEncCreateBitstreamBuffer(BITSTREAM_BUFFER_SIZE, &m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
		if (nvStatus != NV_ENC_SUCCESS)
			return nvStatus;
		m_stEncodeBuffer[i].stOutputBfr.dwBitstreamBufferSize = BITSTREAM_BUFFER_SIZE;
		m_stEncodeBuffer[i].stOutputBfr.bEOSFlag = false;
 		if (encodeConfig.enableAsyncMode)
 		{
 			nvStatus = m_pNvHWEncoder->NvEncRegisterAsyncEvent(&m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
 			if (nvStatus != NV_ENC_SUCCESS)
 				return nvStatus;
 			m_stEncodeBuffer[i].stOutputBfr.bWaitOnEvent = true;
 		}
 		else
			m_stEncodeBuffer[i].stOutputBfr.hOutputEvent = NULL;
	}

	m_stEOSOutputBfr.bEOSFlag = TRUE;

	if (encodeConfig.enableAsyncMode)
	{
		nvStatus = m_pNvHWEncoder->NvEncRegisterAsyncEvent(&m_stEOSOutputBfr.hOutputEvent);
		if (nvStatus != NV_ENC_SUCCESS)
			return nvStatus;
	}
	else
		m_stEOSOutputBfr.hOutputEvent = NULL;

	return NV_ENC_SUCCESS;
}

NVENCSTATUS NVSDKVideoEncoderPrivate::ReleaseIOBuffers()
{
	for (uint32_t i = 0; i < m_uEncodeBufferCount; i++)
	{
		m_pNvHWEncoder->NvEncDestroyInputBuffer(m_stEncodeBuffer[i].stInputBfr.hInputSurface);
		m_stEncodeBuffer[i].stInputBfr.hInputSurface = NULL;
		m_pNvHWEncoder->NvEncDestroyBitstreamBuffer(m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
		m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer = NULL;
		if (encodeConfig.enableAsyncMode)
		{
			m_pNvHWEncoder->NvEncUnregisterAsyncEvent(m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
			nvCloseFile(m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
			m_stEncodeBuffer[i].stOutputBfr.hOutputEvent = NULL;
		}
	}

	if (m_stEOSOutputBfr.hOutputEvent)
	{
		if (encodeConfig.enableAsyncMode)
		{
			m_pNvHWEncoder->NvEncUnregisterAsyncEvent(m_stEOSOutputBfr.hOutputEvent);
			nvCloseFile(m_stEOSOutputBfr.hOutputEvent);
			m_stEOSOutputBfr.hOutputEvent = NULL;
		}
	}

	return NV_ENC_SUCCESS;
}
NVENCSTATUS NVSDKVideoEncoderPrivate::Deinitialize(uint32_t devicetype)
{
	NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

// 	if (m_stEncoderInput.enableMEOnly)
// 	{
// 		ReleaseMVIOBuffers();
// 	}
// 	else
// 	{
		ReleaseIOBuffers();
//	}

	nvStatus = m_pNvHWEncoder->NvEncDestroyEncoder();

	if (m_pDevice)
	{
		switch (devicetype)
		{
#if defined(NV_WINDOWS)
		case NV_ENC_DX9:
			((IDirect3DDevice9*)(m_pDevice))->Release();
			break;

		case NV_ENC_DX10:
			((ID3D10Device*)(m_pDevice))->Release();
			break;

		case NV_ENC_DX11:
			((ID3D11Device*)(m_pDevice))->Release();
			break;
#endif

		case NV_ENC_CUDA:
			CUresult cuResult = CUDA_SUCCESS;
			cuResult = cuCtxDestroy((CUcontext)m_pDevice);
			if (cuResult != CUDA_SUCCESS)
				PRINTERR("cuCtxDestroy error:0x%x\n", cuResult);
		}

		m_pDevice = NULL;
	}

#if defined (NV_WINDOWS)
	if (m_pD3D)
	{
		m_pD3D->Release();
		m_pD3D = NULL;
	}
#endif

	return nvStatus;
}
NVENCSTATUS NVSDKVideoEncoderPrivate::EncodeFrame(EncodeFrameConfig *pEncodeFrame, bool bFlush, uint32_t width, uint32_t height)
{
	NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
	uint32_t lockedPitch = 0;
	EncodeBuffer *pEncodeBuffer = NULL;

	if (bFlush)
	{
		return FlushEncoder();
	}

	if (!pEncodeFrame)
	{
		return NV_ENC_ERR_INVALID_PARAM;
	}

	pEncodeBuffer = m_EncodeBufferQueue.GetAvailable();
	if(!pEncodeBuffer)
	{
		//m_pNvHWEncoder->ProcessOutput(m_EncodeBufferQueue.GetPending());
		VideoOutputFrame encFrame = getEncodedFrame(m_EncodeBufferQueue.GetPending());
		if(encFrame.frameData!=NULL && encFrame.frameSize>0)
		{
			m_encodedFrames.push_back(encFrame);
		}
		pEncodeBuffer = m_EncodeBufferQueue.GetAvailable();
	}

	unsigned char *pInputSurface;

	nvStatus = m_pNvHWEncoder->NvEncLockInputBuffer(pEncodeBuffer->stInputBfr.hInputSurface, (void**)&pInputSurface, &lockedPitch);
	if (nvStatus != NV_ENC_SUCCESS)
		return nvStatus;

	if (pEncodeBuffer->stInputBfr.bufferFmt == NV_ENC_BUFFER_FORMAT_NV12_PL)
	{
		unsigned char *pInputSurfaceCh = pInputSurface + (pEncodeBuffer->stInputBfr.dwHeight*lockedPitch);
		//convertYUVpitchtoNV12(pEncodeFrame->yuv[0], pEncodeFrame->yuv[1], pEncodeFrame->yuv[2], pInputSurface, pInputSurfaceCh, width, height, width, lockedPitch);
		copyNV12(pEncodeFrame->yuv[0], pEncodeFrame->yuv[1], pEncodeFrame->stride[0], pEncodeFrame->stride[1],
			pInputSurface, pInputSurfaceCh, lockedPitch, lockedPitch, width, height);
	}
	else if (pEncodeBuffer->stInputBfr.bufferFmt == NV_ENC_BUFFER_FORMAT_YUV444)
	{
		unsigned char *pInputSurfaceCb = pInputSurface + (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
		unsigned char *pInputSurfaceCr = pInputSurfaceCb + (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
		convertYUVpitchtoYUV444(pEncodeFrame->yuv[0], pEncodeFrame->yuv[1], pEncodeFrame->yuv[2], pInputSurface, pInputSurfaceCb, pInputSurfaceCr, width, height, width, lockedPitch);
	}
	else if (pEncodeBuffer->stInputBfr.bufferFmt == NV_ENC_BUFFER_FORMAT_YUV420_10BIT)
	{
		unsigned char *pInputSurfaceCh = pInputSurface + (pEncodeBuffer->stInputBfr.dwHeight*lockedPitch);
		convertYUV10pitchtoP010PL((uint16_t *)pEncodeFrame->yuv[0], (uint16_t *)pEncodeFrame->yuv[1], (uint16_t *)pEncodeFrame->yuv[2], (uint16_t *)pInputSurface, (uint16_t *)pInputSurfaceCh, width, height, width, lockedPitch);
	}
	else
	{
		unsigned char *pInputSurfaceCb = pInputSurface + (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
		unsigned char *pInputSurfaceCr = pInputSurfaceCb + (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
		convertYUV10pitchtoYUV444((uint16_t *)pEncodeFrame->yuv[0], (uint16_t *)pEncodeFrame->yuv[1], (uint16_t *)pEncodeFrame->yuv[2], (uint16_t *)pInputSurface, (uint16_t *)pInputSurfaceCb, (uint16_t *)pInputSurfaceCr, width, height, width, lockedPitch);
	}
	nvStatus = m_pNvHWEncoder->NvEncUnlockInputBuffer(pEncodeBuffer->stInputBfr.hInputSurface);
	if (nvStatus != NV_ENC_SUCCESS)
		return nvStatus;

	nvStatus = m_pNvHWEncoder->NvEncEncodeFrame(pEncodeBuffer, NULL, width, height, (NV_ENC_PIC_STRUCT)m_uPicStruct, pEncodeFrame->qpDeltaMapArray, pEncodeFrame->qpDeltaMapArraySize, pEncodeFrame->meExternalHints, pEncodeFrame->meHintCountsPerBlock);
	return nvStatus;
}

NVENCSTATUS NVSDKVideoEncoderPrivate::FlushEncoder()
{
	NVENCSTATUS nvStatus = m_pNvHWEncoder->NvEncFlushEncoderQueue(m_stEOSOutputBfr.hOutputEvent);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		assert(0);
		return nvStatus;
	}

	nvStatus = NV_ENC_ERR_GENERIC;
	do 
	{
		EncodeBuffer *pEncodeBufer = m_EncodeBufferQueue.GetPending();
		Media::VideoOutputFrame encodedFrame = getEncodedFrame(pEncodeBufer);
		if(encodedFrame.frameSize<=0 || encodedFrame.frameData==NULL)
			break;
		nvStatus = NV_ENC_SUCCESS;
		m_encodedFrames.push_back(encodedFrame);
	} while (true);
/*	EncodeBuffer *pEncodeBufer = m_EncodeBufferQueue.GetPending();
	while (pEncodeBufer)
	{
		m_pNvHWEncoder->ProcessOutput(pEncodeBufer);
		pEncodeBufer = m_EncodeBufferQueue.GetPending();
	}
*/
#if defined(NV_WINDOWS)
	if (encodeConfig.enableAsyncMode)
	{

		if (WaitForSingleObject(m_stEOSOutputBfr.hOutputEvent, 500) != WAIT_OBJECT_0)
		{
			assert(0);
			nvStatus = NV_ENC_ERR_GENERIC;
		}
	}
#endif  

	return nvStatus;
}

Media::VideoOutputFrame NVSDKVideoEncoderPrivate::getEncodedFrame(EncodeBuffer *pEncodeBufer)
{
	Media::VideoOutputFrame encodedFrame(NULL, 0);
	if (pEncodeBufer)
	{
		uint32_t buflen = 1024 * 1024 * 2;
		uint8_t* buffer = (uint8_t*)malloc(buflen);
		uint64_t pts = 0;
		NV_ENC_PIC_TYPE picType = NV_ENC_PIC_TYPE_UNKNOWN;
		NVENCSTATUS result = m_pNvHWEncoder->ProcessOutput(pEncodeBufer, buffer, &buflen, &pts, &picType);
		if(NV_ENC_SUCCESS!=result || buflen<=0)
		{
			free(buffer);
			return encodedFrame;
		}
		encodedFrame.frameData = (unsigned char*)buffer;
		encodedFrame.frameSize = buflen;
		encodedFrame.pts = pts;
		switch(picType)
		{
		case NV_ENC_PIC_TYPE_I:
			encodedFrame.frameType = FRAME_TYPE_I;
			break;
		case NV_ENC_PIC_TYPE_P:
			encodedFrame.frameType = FRAME_TYPE_P;
			break;
		case NV_ENC_PIC_TYPE_B:
			encodedFrame.frameType = FRAME_TYPE_B;
			break;
		case NV_ENC_PIC_TYPE_IDR:
			encodedFrame.frameType = FRAME_TYPE_IDR;
			break;
		}
	}
	return encodedFrame;
}

#endif