/** 
 *	@file		libobssqv11.h
 *	@author		zhuqingquan
 *	@brief		Helper function to load obs-qsv11.dll in runtime£¬ and import the interface of obs-qsv11.dll
 *	@created	2018/06/05
 **/
#pragma once
#ifndef _MEDIA_LIB_OBS_SQV_11_H_
#define _MEDIA_LIB_OBS_SQV_11_H_

#include <string>
#include "QSV_Encoder.h"

namespace Media
{
	class libobsqsv11
	{
	public:
		// load dll
		static int init(const std::wstring& dllFilePathName);
		static void uninit();
		// interface in obs-qsv11.dll
		static int qsv_encoder_close(qsv_t *);
// 		static int qsv_param_parse(qsv_param_t *, const char *name, const char *value);
// 		static int qsv_param_apply_profile(qsv_param_t *, const char *profile);
// 		static int qsv_param_default_preset(qsv_param_t *, const char *preset,
//			const char *tune);
		static int qsv_encoder_reconfig(qsv_t *, qsv_param_t *);
		static void qsv_encoder_version(unsigned short *major, unsigned short *minor);
		static qsv_t *qsv_encoder_open( qsv_param_t * );
		static int qsv_encoder_encode(qsv_t *, uint64_t, uint8_t *, uint8_t *, uint32_t,
			uint32_t, mfxBitstream **pBS);
		static int qsv_encoder_headers(qsv_t *, uint8_t **pSPS, uint8_t **pPPS,
			uint16_t *pnSPS, uint16_t *pnPPS);
		static enum qsv_cpu_platform qsv_get_cpu_platform();
	};
}

#endif //_MEDIA_LIB_OBS_SQV_11_H_