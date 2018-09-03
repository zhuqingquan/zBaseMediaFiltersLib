#ifndef _MEDIA_FILTER_COMMON_FORCC_H_
#define _MEDIA_FILTER_COMMON_FORCC_H_

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#elif __linux__
#define mmioFOURCC(ch0, ch1, ch2, ch3) \
    (((uint32_t)(uint8_t)(ch0)) | ((uint32_t)(uint8_t)(ch1) << 8) |\
    ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24))
#endif

namespace zMedia
{
		//FOURCC define
		/* yuyu		4:2:2 16bit, y-u-y-v, packed*/
#define FOURCC_YUY2	mmioFOURCC('Y','U','Y','2')
#define FOURCC_YUYV	mmioFOURCC('Y','U','Y','V')
		/* uyvy		4:2:2 16bit, u-y-v-y, packed */
#define FOURCC_UYVY	mmioFOURCC('U','Y','V','Y')
		/* i420		y-u-v, planar */
#define FOURCC_I420	mmioFOURCC('I','4','2','0')
#define FOURCC_IYUV	mmioFOURCC('I','Y','U','V')
		/* yv12		y-v-u, planar */
#define FOURCC_YV12	mmioFOURCC('Y','V','1','2')

#define FOURCC_HDYC mmioFOURCC('H','D','Y','C') //等同于FOURCC_UYVY，但颜色范围不同，先忽略不计
}

#endif// _MEDIA_FILTER_COMMON_FORCC_H_
