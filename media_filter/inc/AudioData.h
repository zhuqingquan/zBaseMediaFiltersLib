#ifndef _MEDIA_FILTER_AUDIO_DATA_H_
#define _MEDIA_FILTER_AUDIO_DATA_H_

#include "CodecMediaData.h"
#include "PcmData.h"
#include "mediadata_tpl.h"

namespace zMedia
{
    typedef mediadata_tpl<AudioCodec, PcmData> AudioData;
}//namespace zMedia
#endif//_MEDIA_FILTER_AUDIO_DATA_H_
