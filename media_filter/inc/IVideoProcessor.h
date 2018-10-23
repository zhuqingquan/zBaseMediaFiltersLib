/**
 *	@name		IVideoProcessor.h
 *	@author		zhuqingquan
 *	@brief		Defined the interface of image processor
 **/
#pragma once
#ifndef _Z_MEDIA_I_VIDEO_PROCESSOR_H_
#define _Z_MEDIA_I_VIDEO_PROCESSOR_H_

#include <string>
#include "VideoData.h"

namespace zMedia
{
	/**
	*	@name		IVideoProcessor
	*	@brief		视频数据处理模块的接口
	**/
	class IVideoProcessor
	{
	public:
		IVideoProcessor(const wchar_t* type, const wchar_t* name)
			: m_type(type ? type : L"")
			, m_name(name ? name : L"")
			, m_enable(true)
		{}

		virtual ~IVideoProcessor() = 0 {};

		virtual void setEnable(bool enable) { m_enable = enable; }
		bool enable() const { return m_enable; }

		const wchar_t* getName() const { return m_name.c_str(); }
		const wchar_t* getType() const { return m_type.c_str(); }

		virtual int process(VideoData::SPtr& vframe, VideoData::SPtr* ppOutVFrame) = 0;

	protected:
		long m_enable;			//是否启用，默认true
		std::wstring m_name;
		std::wstring m_type;
	};
}//namespace zMedia

#endif//_Z_MEDIA_I_VIDEO_PROCESSOR_H_