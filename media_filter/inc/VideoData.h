#ifndef _MEDIA_FILTER_VIDEO_DATA_H_
#define _MEDIA_FILTER_VIDEO_DATA_H_

#include "PictureInfo.h"
#include "CodecMediaData.h"
#include "mediadata_tpl.h"

namespace zMedia
{
    typedef mediadata_tpl<PictureCodec, PictureRaw> VideoData;
}//namespace zMedia
#endif// _MEDIA_FILTER_VIDEO_DATA_H_
