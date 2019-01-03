#include "x264Encoder.h"
#include <inttypes.h>
#include "x264.h"
#include <list>
#include <Windows.h>
#include "AudioData.h"
#include "TextHelper.h"

#pragma comment(lib, "x264.lib")

using namespace Media;

namespace Media
{
	struct x264EncoderPrivate
	{
		x264_param_t			params;
		x264_t*					context;
		uint8_t*				extra_data;
		unsigned int			extra_data_size;
		uint8_t*				sei;
		unsigned int			sei_size;
		std::list<VideoOutputFrame> encodedFrames;

		x264EncoderPrivate()
			: context(NULL), extra_data_size(0), extra_data(NULL)
			, sei(NULL), sei_size(0)
		{
			//memset(&params, 0, sizeof(x264_param_t));
			x264_param_default(&params);
		}
	};
}


static void log_x264(void *param, int level, const char *format, va_list args)
{
	//struct obs_x264 *obsx264 = param;
	char str[1024];

	vsnprintf(str, 1024, format, args);
	//info("%s", str);
	OutputDebugStringA(str);

	//UNUSED_PARAMETER(level);
}

bool isSurpportPixelFormat(zMedia::E_PIXFMT pixfmt);

x264Encoder::x264Encoder()
: m_private(new x264EncoderPrivate())
{

}

x264Encoder::~x264Encoder()
{
	delete m_private;
	m_private = NULL;
}

bool x264Encoder::getValidPixelFormat( int* pixfmt, int& count )
{
	if(count<2)	return false;
	pixfmt[0] = (int)zMedia::PIXFMT_E_I420;
	pixfmt[1] = (int)zMedia::PIXFMT_E_NV12;
	count = 2;
	return true;
}

bool x264Encoder::init()
{
	if(m_private->context!=NULL)
		return false;
	// 需要先调用setConfig再调用init才能成功，否则宽高信息不存在
	if(m_private->params.i_width<=0 || m_private->params.i_height<=0)
		return false;
	m_private->context = x264_encoder_open(&m_private->params);

	if (m_private->context == NULL)
	{
		//fixme log
		//warn("x264 failed to load");
	}
	else
	{
		loadHeaders();
	}
	return true;
}

static const char *validate(const char *val, const char *name,
							const char *const *list)
{
	if (!val || !*val)
		return val;

	while (*list) {
		if (strcmp(val, *list) == 0)
			return val;

		list++;
	}

	//warn("Invalid %s: %s", name, val);
	return NULL;
}

static inline const char *validate_preset(const char *preset)
{
	const char *new_preset = validate(preset, "preset",
		x264_preset_names);
	return new_preset ? new_preset : "veryfast";
}

static bool reset_x264_params(struct x264EncoderPrivate *obsx264,
							  const char *preset, const char *tune)
{
	int ret = x264_param_default_preset(&obsx264->params,
		validate_preset(preset),
		validate(tune, "tune", x264_tune_names));
	return ret == 0;
}

void *bmemdup(const void *ptr, size_t size)
{
	void *out = malloc(size);
	if (size)
		memcpy(out, ptr, size);

	return out;
}

static inline char *bstrdup_n(const char *str, size_t n)
{
	char *dup;
	if (!str)
		return NULL;

	dup = (char*)bmemdup(str, n+1);
	dup[n] = 0;

	return dup;
}

static inline char *bstrdup(const char *str)
{
	if (!str)
		return NULL;

	return bstrdup_n(str, strlen(str));
}

static bool getparam(const char *param, char **name, const char **value)
{
	const char *assign;

	if (!param || !*param || (*param == '='))
		return false;

	assign = strchr(param, '=');
	if (!assign || !*assign || !*(assign+1))
		return false;

	*name  = bstrdup_n(param, assign-param);
	*value = assign+1;
	return true;
}

static void override_base_param(const char *param,
								char **preset, char **profile, char **tune)
{
	char       *name;
	const char *val;

	if (getparam(param, &name, &val)) {
		if (zUtils::astrcmpi(name, "preset") == 0) {
			const char *valid_name = validate(val,
				"preset", x264_preset_names);
			if (valid_name) {
				free(*preset);
				*preset = bstrdup(val);
			}

		} else if (zUtils::astrcmpi(name, "profile") == 0) {
			const char *valid_name = validate(val,
				"profile", x264_profile_names);
			if (valid_name) {
				free(*profile);
				*profile = bstrdup(val);
			}

		} else if (zUtils::astrcmpi(name, "tune") == 0) {
			const char *valid_name = validate(val,
				"tune", x264_tune_names);
			if (valid_name) {
				free(*tune);
				*tune = bstrdup(val);
			}
		}

		free(name);
	}
}

static inline void override_base_params(char **params,
										char **preset, char **profile, char **tune)
{
	while (*params)
		override_base_param(*(params++),
		preset, profile, tune);
}

struct video_scale_info {
	zMedia::E_PIXFMT     format;
	uint32_t              width;
	uint32_t              height;
	enum zMedia::video_range_type range;
	enum zMedia::video_colorspace colorspace;
};

static inline const char *get_x264_colorspace_name(enum zMedia::video_colorspace cs)
{
	switch (cs) {
		case zMedia::VIDEO_CS_DEFAULT:
		case zMedia::VIDEO_CS_601:
			return "undef";
		case zMedia::VIDEO_CS_709:;
	}

	return "bt709";
}

static inline int get_x264_cs_val(enum zMedia::video_colorspace cs,
								  const char *const names[])
{
	const char *name = get_x264_colorspace_name(cs);
	int idx = 0;
	do {
		if (strcmp(names[idx], name) == 0)
			return idx;
	} while (!!names[++idx]);

	return 0;
}

enum rate_control {
	RATE_CONTROL_CBR,
	RATE_CONTROL_VBR,
	RATE_CONTROL_ABR,
	RATE_CONTROL_CRF
};

#define OPENCL_ALIAS "opencl_is_experimental_and_potentially_unstable"

static inline void set_param(const char *param, x264_param_t& x264Params)
{
	char       *name;
	const char *val;

	if (getparam(param, &name, &val)) {
		if (strcmp(name, "preset")    != 0 &&
			strcmp(name, "profile")   != 0 &&
			strcmp(name, "tune")      != 0 &&
			strcmp(name, "fps")       != 0 &&
			strcmp(name, "force-cfr") != 0 &&
			strcmp(name, "width")     != 0 &&
			strcmp(name, "height")    != 0 &&
			strcmp(name, "opencl")    != 0) {
				if (strcmp(name, OPENCL_ALIAS) == 0)
					strcpy(name, "opencl");
				if (x264_param_parse(&x264Params, name, val) != 0)
				{
					//warn("x264 param: %s failed", param);
				}
		}

		free(name);
	}
}

static void update_params(const VideoEncoderConfig& vCfg, x264_param_t& x264Params, char **params)
{
	//video_t *video = obs_encoder_video(obsx264->encoder);
	//const struct video_output_info *voi = video_output_get_info(video);
	struct video_scale_info info;

	info.format = (zMedia::E_PIXFMT)vCfg.pixfmt;//->format;
	info.colorspace = (zMedia::video_colorspace)vCfg.colorspace;
	info.range = (zMedia::video_range_type)vCfg.range_type;

//	obs_x264_video_info(obsx264, &info);

//	const char *rate_control = obs_data_get_string(settings, "rate_control");

	int bitrate      = vCfg.maxBitrate / 1000;//(int)obs_data_get_int(settings, "bitrate");
	int buffer_size  = bitrate;//(int)obs_data_get_int(settings, "buffer_size");
	int keyint_sec   = vCfg.gop;//(int)obs_data_get_int(settings, "keyint_sec");
	int crf          = 23;//根据tagRcMode的定义，现在还用不到这个值，使用默认值
	int width        = vCfg.width;//(int)obs_encoder_get_width(obsx264->encoder);
	int height       = vCfg.height;//(int)obs_encoder_get_height(obsx264->encoder);
	int bf           = vCfg.numBFrame;//(int)obs_data_get_int(settings, "bf");
//	bool use_bufsize = obs_data_get_bool(settings, "use_bufsize");
//	bool cbr_override= obs_data_get_bool(settings, "cbr");
	enum rate_control rc;

// #ifdef ENABLE_VFR
// 	bool vfr         = obs_data_get_bool(settings, "vfr");
// #endif

	/* XXX: "cbr" setting has been deprecated */
// 	if (cbr_override) {
// 		warn("\"cbr\" setting has been deprecated for all encoders!  "
// 		     "Please set \"rate_control\" to \"CBR\" instead.  "
// 		     "Forcing CBR mode.  "
// 		     "(Note to all: this is why you shouldn't use strings for "
// 		     "common settings)");
// 		rate_control = "CBR";
// 	}

// 	if (astrcmpi(rate_control, "ABR") == 0) {
// 		rc = RATE_CONTROL_ABR;
// 		crf = 0;
// 
// 	} else if (astrcmpi(rate_control, "VBR") == 0) {
// 		rc = RATE_CONTROL_VBR;
// 
// 	} else if (astrcmpi(rate_control, "CRF") == 0) {
// 		rc = RATE_CONTROL_CRF;
// 		bitrate = 0;
// 		buffer_size = 0;
// 
// 	} else { /* CBR */
// 		rc = RATE_CONTROL_CBR;
// 		crf = 0;
// 	}

// 	if (keyint_sec)
// 		obsx264->params.i_keyint_max =
// 			keyint_sec * voi->fps_num / voi->fps_den;
	x264Params.i_keyint_max = vCfg.gop;
	switch(vCfg.rcMode)
	{
	case VideoEncoderConfig::RC_CBR:
		rc = RATE_CONTROL_CBR;
		crf = 0;
		break;
	case VideoEncoderConfig::RC_VBR:
		rc = RATE_CONTROL_VBR;
		break;
	case VideoEncoderConfig::RC_CQP:
		break;
	default:
		rc = RATE_CONTROL_CBR;
		crf = 0;
		break;
	}
	

//	if (!use_bufsize)
//		buffer_size = bitrate;

// #ifdef ENABLE_VFR
// 	obsx264->params.b_vfr_input          = vfr;
// #else
// 	obsx264->params.b_vfr_input          = false;
// #endif
	x264Params.b_vfr_input			= false;
	x264Params.rc.i_vbv_max_bitrate = bitrate;
	x264Params.rc.i_vbv_buffer_size = buffer_size;
	x264Params.rc.i_bitrate         = bitrate;
	x264Params.i_width              = width;
	x264Params.i_height             = height;
	x264Params.i_fps_num            = vCfg.fps;//voi->fps_num;
	x264Params.i_fps_den            = 1;//voi->fps_den;
	x264Params.pf_log               = log_x264;
//	x264Params.p_log_private        = obsx264;
	x264Params.i_log_level          = X264_LOG_WARNING;

//	if (obs_data_has_user_value(settings, "bf"))
//		obsx264->params.i_bframe = bf;
	x264Params.i_bframe = bf;

	x264Params.vui.i_transfer =
		get_x264_cs_val((zMedia::video_colorspace)vCfg.colorspace, x264_transfer_names);
	x264Params.vui.i_colmatrix =
		get_x264_cs_val((zMedia::video_colorspace)vCfg.colorspace, x264_colmatrix_names);
	x264Params.vui.i_colorprim =
		get_x264_cs_val((zMedia::video_colorspace)vCfg.colorspace, x264_colorprim_names);
	x264Params.vui.b_fullrange = vCfg.range_type == zMedia::VIDEO_RANGE_FULL;

	/* use the new filler method for CBR to allow real-time adjusting of
	 * the bitrate */
	if (rc == RATE_CONTROL_CBR || rc == RATE_CONTROL_ABR) {
		x264Params.rc.i_rc_method   = X264_RC_ABR;

		if (rc == RATE_CONTROL_CBR) {
#if X264_BUILD >= 139
			x264Params.rc.b_filler = true;
#else
			x264Params.i_nal_hrd = X264_NAL_HRD_CBR;
#endif
		}
	} else {
		x264Params.rc.i_rc_method   = X264_RC_CRF;
	}

	x264Params.rc.f_rf_constant = (float)crf;		//此处crf现在为止应该都为0

	switch (vCfg.pixfmt)
	{
	case zMedia::PIXFMT_E_NV12:
		x264Params.i_csp = X264_CSP_NV12;
		break;
	case zMedia::PIXFMT_E_I420:
		x264Params.i_csp = X264_CSP_I420;
		break;
	default:
		x264Params.i_csp = X264_CSP_NV12;
		break;
	}
// 	if (info.format == VIDEO_FORMAT_NV12)
// 		obsx264->params.i_csp = X264_CSP_NV12;
// 	else if (info.format == VIDEO_FORMAT_I420)
// 		obsx264->params.i_csp = X264_CSP_I420;
// 	else if (info.format == VIDEO_FORMAT_I444)
// 		obsx264->params.i_csp = X264_CSP_I444;
// 	else
// 		obsx264->params.i_csp = X264_CSP_NV12;

	while (*params)
		set_param(*(params++), x264Params);

// 	info("settings:\n"
// 	     "\trate_control: %s\n"
// 	     "\tbitrate:      %d\n"
// 	     "\tbuffer size:  %d\n"
// 	     "\tcrf:          %d\n"
// 	     "\tfps_num:      %d\n"
// 	     "\tfps_den:      %d\n"
// 	     "\twidth:        %d\n"
// 	     "\theight:       %d\n"
// 	     "\tkeyint:       %d\n",
// 	     rate_control,
// 	     obsx264->params.rc.i_vbv_max_bitrate,
// 	     obsx264->params.rc.i_vbv_buffer_size,
// 	     (int)obsx264->params.rc.f_rf_constant,
// 	     voi->fps_num, voi->fps_den,
// 	     width, height,
// 	     obsx264->params.i_keyint_max);
}

static inline void apply_x264_profile(x264_t* context, x264_param_t& x264Params,
									  const char *profile)
{
	if (!context && profile && *profile) {
		int ret = x264_param_apply_profile(&x264Params, profile);
		if (ret != 0)
		{
			//warn("Failed to set x264 profile '%s'", profile);
		}
	}
}

bool x264Encoder::setConfig( const VideoEncoderConfig& vCfg )
{
	char **paramlist;
	bool success = true;
	
	paramlist = zUtils::strlist_split(vCfg.configString, ' ', false);

	//fixme should get from paramlist and Default
	char *preset     = bstrdup("fast");
	char *profile    = bstrdup("baseline");
	char *tune       = bstrdup("psnr");//bstrdup("zerolatency");

	if (!m_private->context) {
		override_base_params(paramlist,
			&preset, &profile, &tune);

		//if (preset  && *preset)  info("preset: %s",  preset);
		//if (profile && *profile) info("profile: %s", profile);
		//if (tune    && *tune)    info("tune: %s",    tune);

		success = reset_x264_params(m_private, preset, tune);
	}
	if (success) {
		update_params(vCfg, m_private->params, paramlist);

		if (!m_private->context)
			apply_x264_profile(m_private->context, m_private->params, profile);
	}
	m_private->params.b_repeat_headers = false;

	zUtils::strlist_free(paramlist);
	free(preset);
	free(profile);
	free(tune);

	if (success && NULL!=m_private->context) {
		int ret = x264_encoder_reconfig(m_private->context, &m_private->params);
		if (ret != 0)
		{
			//warn("Failed to reconfigure: %d", ret);
		}
		//else
		//{
		//	// reconfig 之后需要重新获取pps、sps、sei等
		//	// 但测试发现，即使分辨率修改了，这些信息可能并未更新
		//	// 推荐使用I帧编码成功后和I帧一起生成的pps、sps、sei
		//	loadHeaders();
		//}
		return ret == 0;
	}
	return true;
}

static inline void init_pic_data(const x264_param_t& params, x264_picture_t *pic, const zMedia::VideoData::SPtr& vframe)
{
	x264_picture_init(pic);

	zMedia::PictureRaw::SPtr picRaw = vframe->getRawData();
	pic->i_pts = picRaw->getTimestamp();
	pic->img.i_csp = params.i_csp;

	if (params.i_csp == X264_CSP_NV12)
		pic->img.i_plane = 2;
	else if (params.i_csp == X264_CSP_I420)
		pic->img.i_plane = 3;
	else if (params.i_csp == X264_CSP_I444)
		pic->img.i_plane = 3;

	int linesize[4] = {0};
	uint8_t* data[4] = {NULL};
	//Video::PICTURE* picInfo = vframe->getPicInfo();
	switch(picRaw->format().ePixfmt)
	{
	case zMedia::PIXFMT_E_I420:
		data[0] = (uint8_t*)picRaw->y();
		data[1] = (uint8_t*)picRaw->u();
		data[2] = (uint8_t*)picRaw->v();
		linesize[0] = picRaw->format().y_stride;
		linesize[1] = picRaw->format().u_stride;
		linesize[2] = picRaw->format().v_stride;
		break;
	case zMedia::PIXFMT_E_NV12:
		data[0] = (uint8_t*)picRaw->y();
		data[1] = (uint8_t*)picRaw->uv();
		linesize[0] = picRaw->format().y_stride;
		linesize[1] = picRaw->format().u_stride;
		break;
	default:
		return;
	}

	for (int i = 0; i < pic->img.i_plane; i++) {
		pic->img.i_stride[i] = linesize[i];
		pic->img.plane[i]    = data[i];
	}
}

void get_x264_picture_info(x264_nal_t* nals, int nal_count, int& frame_size, ENCODED_VIDEO_FRAME_TYPE& frame_type, bool& havePPSSPS)
{
	int count = 0;
	bool havePPS = false;
	bool haveSPS = false;
	for (int i = 0; i < nal_count; i++) 
	{
		x264_nal_t* nal = nals+i;
		switch(nal->i_type)
		{
		case NAL_SLICE_IDR:
			frame_type = FRAME_TYPE_IDR;	//可能I帧
			count += nal->i_payload;
			break;
		case NAL_SEI:
			count += nal->i_payload;
			break;
		case NAL_SPS:
			//encodedFrame.nalType = FRAME_TYPE_IDR;
			haveSPS = true;
			count += nal->i_payload;
			break;
		case NAL_PPS:
			//encodedFrame.nalType = FRAME_TYPE_IDR;
			havePPS = true;
			count += nal->i_payload;
			break;
		case NAL_SLICE:
			frame_type = FRAME_TYPE_P;		//可能是P、B帧
			count += nal->i_payload;
			break;
		case NAL_FILLER:
			break;
		default:
			break;
			//encodedFrame.frameType;
		}
	}
	havePPSSPS = havePPS && haveSPS;
	frame_size = count;
}
static VideoOutputFrame parse_packet(x264_nal_t *nal, x264_picture_t *pic_out)
{
	VideoOutputFrame encodedFrame(NULL, 0);
	if (!nal) return encodedFrame;

	encodedFrame.frameData = (unsigned char*)malloc(nal->i_payload);
	memcpy(encodedFrame.frameData, (uint8_t*)nal->p_payload, nal->i_payload);
	encodedFrame.frameSize = nal->i_payload;
	encodedFrame.pts = (uint32_t)pic_out->i_pts;
	encodedFrame.dts = (uint32_t)pic_out->i_dts;
	encodedFrame.nalRefIdc = nal->i_ref_idc;
	switch(nal->i_type)
	{
	case NAL_SLICE_IDR:
		encodedFrame.frameType = FRAME_TYPE_IDR;
		break;
	case NAL_SEI:
		break;
	case NAL_SPS:
		//encodedFrame.nalType = FRAME_TYPE_IDR;
		break;
	case NAL_PPS:
		//encodedFrame.nalType = FRAME_TYPE_IDR;
		break;
	default:
		break;
		//encodedFrame.frameType;
	}

	//encodedFrame.nalType;
	//packet->keyframe      = pic_out->b_keyframe != 0;
	return encodedFrame;
}

bool x264Encoder::encodeFrame(zMedia::VideoData::SPtr vframe )
{
	if(vframe)
	{
		zMedia::PictureRaw::SPtr picRaw = vframe->getRawData();
		if (NULL == picRaw || !isSurpportPixelFormat(picRaw->format().ePixfmt))
		{
			// maybe log here
			return false;
		}
	}
	x264_nal_t      *nals;
	int             nal_count;
	int             ret;
	x264_picture_t  pic, pic_out;

	if(NULL!=vframe)
	{
		init_pic_data(m_private->params, &pic, vframe);
	}

	ret = x264_encoder_encode(m_private->context, &nals, &nal_count, (vframe ? &pic : NULL), &pic_out);
	if (ret < 0) {
		//warn("encode failed");
		return false;
	}

	int frame_len = 0;
	ENCODED_VIDEO_FRAME_TYPE frame_type = FRAME_TYPE_UNKNOW;
	bool havePPSSPS = false;
	get_x264_picture_info(nals, nal_count, frame_len, frame_type, havePPSSPS);
	if(frame_len<=0)	//编码成功但是没有帧输出
	{
		if(NULL==vframe)
		{
			// 当使用NULL无法获取到编码帧时返回false，表明编码器内不再有缓存未输出的帧
			// 调用者在这种情况下可能不在需要以NULL调用该接口获取缓存的帧
			return false;
		}
		else
		{
			return true;
		}
	}

	bool needAddHeader = false;
	if((pic_out.i_type==X264_TYPE_IDR || pic_out.i_type==X264_TYPE_I) && !havePPSSPS)
	{
		// 当获取到IDR或者I帧，而又没有sps、pps时，帧前面带上sps、pps
		needAddHeader = true;
	}
	int totallen = needAddHeader ? frame_len + m_private->extra_data_size : frame_len;
	VideoOutputFrame encodedFrame(NULL, 0);
	encodedFrame.frameData = (unsigned char*)malloc(totallen);
	int write_pos = 0;
	if(needAddHeader)
	{
		memcpy(encodedFrame.frameData+write_pos, m_private->extra_data, m_private->extra_data_size);
		write_pos += m_private->extra_data_size;
	}
	for (int i = 0; i < nal_count; i++) 
	{
		x264_nal_t* nal = nals+i;
		if(nal->i_type==NAL_FILLER)
			continue;
		//char dbgmsg[128] = {0};
		//sprintf_s(dbgmsg, 128, "NAL length=%d type=%d\n", nal->i_payload, nal->i_type);
		//OutputDebugStringA(dbgmsg);
		memcpy(encodedFrame.frameData+write_pos, nal->p_payload, nal->i_payload);
		write_pos += nal->i_payload;
// 		VideoOutputFrame outputFrame = parse_packet(nals+i, &pic_out);
// 		if(outputFrame.frameData!=NULL || outputFrame.frameSize>0)
// 		{
// 			m_private->encodedFrames.push_back(outputFrame);
// 		}
	}
	encodedFrame.frameSize = frame_len;
	encodedFrame.frameType = frame_type;
	encodedFrame.pts = (uint32_t)pic_out.i_pts;
	encodedFrame.dts = (uint32_t)pic_out.i_dts;
	//encodedFrame.nalRefIdc = nal->i_ref_idc;
	//encodedFrame.nalType;
	switch(pic_out.i_type)
	{
	case X264_TYPE_IDR:
		encodedFrame.frameType = FRAME_TYPE_IDR;
		break;
	case X264_TYPE_I:
		encodedFrame.frameType = FRAME_TYPE_I;
		break;
	case X264_TYPE_P:
		encodedFrame.frameType = FRAME_TYPE_P;
		break;
	case X264_TYPE_B:
		encodedFrame.frameType = FRAME_TYPE_B;
		break;
	case X264_TYPE_BREF:
		break;
	case X264_TYPE_KEYFRAME:
		break;
	default:
		break;
	}
	m_private->encodedFrames.push_back(encodedFrame);
	return true;
}

bool x264Encoder::getEncFrame( VideoOutputFrame& outFrame )
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

int x264Encoder::loadHeaders()
{
	// 此处获取到的SPS、PPS、SEI等信息未必可靠，如果分辨率在编码过程中修改过了并且调用了x264_encoder_reconfig也未必正确
	// 推荐使用I帧编码成功后和I帧一起生成的pps、sps、sei
	x264_nal_t      *nals;
	int             nal_count;

	x264_encoder_headers(m_private->context, &nals, &nal_count);
	if(nal_count<=0)
		return -1;

	int defaultLen_extra = 1024*4;	//默认分配4K内存用于保存sps、pps等
	int defaultLen_sei = 1024*4;	//默认分配4K内存用于保存sei
	if(m_private->extra_data==NULL)
	{
		m_private->extra_data = (uint8_t*)malloc(defaultLen_extra);
	}
	if(m_private->sei==NULL)
	{
		m_private->sei = (uint8_t*)malloc(defaultLen_sei);
	}

	int extr_data_len = 0;
	int sei_len = 0;
	for (int i = 0; i < nal_count; i++) {
		x264_nal_t *nal = nals+i;

		if (nal->i_type == NAL_SEI)
		{
			sei_len += nal->i_payload;
			if(sei_len>defaultLen_sei)
			{
				m_private->sei = (uint8_t*)realloc(m_private->sei, defaultLen_sei+=defaultLen_sei);
			}
			memcpy(m_private->sei+sei_len-nal->i_payload, nal->p_payload, nal->i_payload);
		}
		else
		{
			extr_data_len += nal->i_payload;
			if(extr_data_len>defaultLen_extra)
			{
				m_private->extra_data = (uint8_t*)realloc(m_private->extra_data, defaultLen_extra+=defaultLen_extra);
			}
			memcpy(m_private->extra_data+extr_data_len-nal->i_payload, nal->p_payload, nal->i_payload);
		}
	}
	m_private->extra_data_size = extr_data_len;
	m_private->sei_size = sei_len;
	return 0;
}

bool isSurpportPixelFormat( zMedia::E_PIXFMT pixfmt )
{
	return pixfmt==zMedia::PIXFMT_E_NV12 || pixfmt==zMedia::PIXFMT_E_I420;
}
