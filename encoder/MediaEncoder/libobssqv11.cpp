#include "libobssqv11.h"

using namespace Media;

typedef int (*func_qsv_encoder_close)(qsv_t *);
// typedef int (*func_qsv_param_parse)(qsv_param_t *, const char *name, const char *value);
// typedef int (*func_qsv_param_apply_profile)(qsv_param_t *, const char *profile);
// typedef int (*func_qsv_param_default_preset)(qsv_param_t *, const char *preset,
// 											const char *tune);
typedef int (*func_qsv_encoder_reconfig)(qsv_t *, qsv_param_t *);
typedef void (*func_qsv_encoder_version)(unsigned short *major, unsigned short *minor);
typedef qsv_t * (*func_qsv_encoder_open)( qsv_param_t * );
typedef int (*func_qsv_encoder_encode)(qsv_t *, uint64_t, uint8_t *, uint8_t *, uint32_t,
					   uint32_t, mfxBitstream **pBS);
typedef int (*func_qsv_encoder_headers)(qsv_t *, uint8_t **pSPS, uint8_t **pPPS,
						uint16_t *pnSPS, uint16_t *pnPPS);
typedef enum qsv_cpu_platform (*func_qsv_get_cpu_platform)();

struct ProcHelper
{
	func_qsv_encoder_close				_func_qsv_encoder_close;
	//func_qsv_param_parse				_func_qsv_param_parse;
	//func_qsv_param_apply_profile		_func_qsv_param_apply_profile;
	//func_qsv_param_default_preset		_func_qsv_param_default_preset;
	func_qsv_encoder_reconfig			_func_qsv_encoder_reconfig;
	func_qsv_encoder_version			_func_qsv_encoder_version;
	func_qsv_encoder_open				_func_qsv_encoder_open;
	func_qsv_encoder_encode				_func_qsv_encoder_encode;
	func_qsv_encoder_headers			_func_qsv_encoder_headers;
	func_qsv_get_cpu_platform			_func_qsv_get_cpu_platform;

	ProcHelper()
		: _func_qsv_encoder_close(NULL)
		, _func_qsv_encoder_reconfig(NULL), _func_qsv_encoder_version(NULL)
		, _func_qsv_encoder_open(NULL), _func_qsv_encoder_encode(NULL)
		, _func_qsv_encoder_headers(NULL), _func_qsv_get_cpu_platform(NULL)
	{

	}
};

static HMODULE		dllModul = NULL;
static ProcHelper	procHelper;

int libobsqsv11::init( const std::wstring& dllFilePathName )
{
	//this is not thread safe;
	if(dllModul==NULL)
	{
		HMODULE dll = ::LoadLibraryExW(dllFilePathName.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
		if(dll)
		{
			ProcHelper tmpProcHelper;
			tmpProcHelper._func_qsv_encoder_close	= (func_qsv_encoder_close)::GetProcAddress(dll, "qsv_encoder_close");
			//tmpProcHelper._func_qsv_param_parse	= (func_qsv_param_parse)::GetProcAddress(dll, "qsv_param_parse");
			//tmpProcHelper._func_qsv_param_apply_profile	= (func_qsv_param_apply_profile)::GetProcAddress(dll, "qsv_param_apply_profile");
			//tmpProcHelper._func_qsv_param_default_preset	= (func_qsv_param_default_preset)::GetProcAddress(dll, "qsv_param_default_preset");
			tmpProcHelper._func_qsv_encoder_reconfig	= (func_qsv_encoder_reconfig)::GetProcAddress(dll, "qsv_encoder_reconfig");
			tmpProcHelper._func_qsv_encoder_version	= (func_qsv_encoder_version)::GetProcAddress(dll, "qsv_encoder_version");
			tmpProcHelper._func_qsv_encoder_open	= (func_qsv_encoder_open)::GetProcAddress(dll, "qsv_encoder_open");
			tmpProcHelper._func_qsv_encoder_encode	= (func_qsv_encoder_encode)::GetProcAddress(dll, "qsv_encoder_encode");
			tmpProcHelper._func_qsv_encoder_headers	= (func_qsv_encoder_headers)::GetProcAddress(dll, "qsv_encoder_headers");
			tmpProcHelper._func_qsv_get_cpu_platform	= (func_qsv_get_cpu_platform)::GetProcAddress(dll, "qsv_get_cpu_platform");
			if(NULL!=tmpProcHelper._func_qsv_encoder_close 
				&& NULL!=tmpProcHelper._func_qsv_encoder_reconfig && NULL!=tmpProcHelper._func_qsv_encoder_version
				&& NULL!=tmpProcHelper._func_qsv_encoder_open && NULL!=tmpProcHelper._func_qsv_encoder_encode
				&& NULL!=tmpProcHelper._func_qsv_encoder_headers && NULL!=tmpProcHelper._func_qsv_get_cpu_platform)
			{
				procHelper = tmpProcHelper;
				dllModul = dll;
				return 0;
			}
		}
	}
	return -1;
}

void libobsqsv11::uninit()
{
	FreeLibrary(dllModul);
	dllModul = NULL;
	procHelper = ProcHelper();
}

int libobsqsv11::qsv_encoder_close( qsv_t * context)
{
	if(NULL==procHelper._func_qsv_encoder_close)
	{
		return -1;
	}
	return procHelper._func_qsv_encoder_close(context);
}

// int libobsqsv11::qsv_param_parse( qsv_param_t * param, const char *name, const char *value )
// {
// 	if(NULL==procHelper._func_qsv_param_parse)
// 	{
// 		return -1;
// 	}
// 	return procHelper._func_qsv_param_parse(param, name, value);
// }
// 
// int libobsqsv11::qsv_param_apply_profile( qsv_param_t * param, const char *profile )
// {
// 	if(NULL==procHelper._func_qsv_param_apply_profile)
// 	{
// 		return -1;
// 	}
// 	return procHelper._func_qsv_param_apply_profile(param, profile);
// }
// 
// int libobsqsv11::qsv_param_default_preset( qsv_param_t * param, const char *preset, const char *tune )
// {
// 	if(NULL==procHelper._func_qsv_param_default_preset)
// 	{
// 		return -1;
// 	}
// 	return procHelper._func_qsv_param_default_preset(param, preset, tune);
// }

int libobsqsv11::qsv_encoder_reconfig( qsv_t * qsv, qsv_param_t * param )
{
	if(NULL==procHelper._func_qsv_encoder_reconfig)
	{
		return -1;
	}
	return procHelper._func_qsv_encoder_reconfig(qsv, param);
}

void libobsqsv11::qsv_encoder_version( unsigned short *major, unsigned short *minor )
{
	if(NULL==procHelper._func_qsv_encoder_version)
	{
		return;
	}
	return procHelper._func_qsv_encoder_version(major, minor);
}

qsv_t * libobsqsv11::qsv_encoder_open( qsv_param_t * param)
{
	if(NULL==procHelper._func_qsv_encoder_open)
	{
		return NULL;
	}
	return procHelper._func_qsv_encoder_open(param);
}

int libobsqsv11::qsv_encoder_encode( qsv_t * qsv, uint64_t pts, uint8_t * y, uint8_t * uv, uint32_t ystride, uint32_t uvstride, mfxBitstream **pBS )
{
	if(NULL==procHelper._func_qsv_encoder_encode)
	{
		return -1;
	}
	return procHelper._func_qsv_encoder_encode(qsv, pts, y, uv, ystride, uvstride, pBS);
}

int libobsqsv11::qsv_encoder_headers( qsv_t * qsv, uint8_t **pSPS, uint8_t **pPPS, uint16_t *pnSPS, uint16_t *pnPPS )
{
	if(NULL==procHelper._func_qsv_encoder_headers)
	{
		return -1;
	}
	return procHelper._func_qsv_encoder_headers(qsv, pSPS, pPPS, pnSPS, pnPPS);
}

enum qsv_cpu_platform libobsqsv11::qsv_get_cpu_platform()
{
	if(NULL==procHelper._func_qsv_get_cpu_platform)
	{
		return QSV_CPU_PLATFORM_UNKNOWN;
	}
	return procHelper._func_qsv_get_cpu_platform();
}
