//#pragma once
#ifndef _MEDIA_FILTER_PICTURE_RAW_H_
#define _MEDIA_FILTER_PICTURE_RAW_H_

#include <assert.h>
#include <limits.h>
#include "MediaData.h"
#include "fourcc.h"
#include "mediafilter.h"


namespace zMedia
{
		//色彩空间 define
		enum E_PIXFMT
		{
			PIXFMT_E_NONE = 0,
			PIXFMT_E_YV12,  // yvu yvu
			PIXFMT_E_I420,	//等同FOURCC_IYUV  //yuv yuv
			PIXFMT_E_YUY2,	//等同FOURCC_YUYV
			PIXFMT_E_UYVY,
			PIXFMT_E_RGB565,   
			PIXFMT_E_RGB24,  //BGR
			PIXFMT_E_RGB32,  //BGRA BGRA
			PIXFMT_E_RGBA,   //RGBA
            PIXFMT_E_BGRA,   //BGRA
			PIXFMT_E_MAX,
		};

		//单个像素的字节个数
		//需配合E_PIXFMT使用，如 PixelBytesCount[PIXFMT_E_RGB32], 若E_PIXFMT的修改导致PixelBytesCount[n]出错，则应同步修改PixelBytesCount
		static const float PixelBytesCount[] = {0, 1.5, 1.5, 2, 2, 2, 3, 4, 4, 4, 4};

		struct PICTURE_FORMAT
		{
			PICTURE_FORMAT()
                : w(0),h(0),ePixfmt(PIXFMT_E_NONE)
                , stride(0),u_stride(0), v_stride(0), a_stride(0)
                , pts(0)
            {}

			PICTURE_FORMAT(int width, int height, E_PIXFMT colorspace)
				: w(width), h(height), ePixfmt(colorspace)
				, stride(0), u_stride(0), v_stride(0), a_stride(0)
                                , pts(0)
			{
				switch(ePixfmt)
				{
				case PIXFMT_E_YUY2:
				case PIXFMT_E_UYVY:
				case PIXFMT_E_RGB565:
				case PIXFMT_E_RGB24:
				case PIXFMT_E_RGB32:
				case PIXFMT_E_RGBA:
                case PIXFMT_E_BGRA:
				case PIXFMT_E_MAX:
					y_stride =Align16Bytes((int) (w * PixelBytesCount[ePixfmt]));
					break;
				case PIXFMT_E_YV12:
				case PIXFMT_E_I420:
					y_stride = Align16Bytes(w);
					u_stride = Align16Bytes(w>>1);//w/2
					v_stride = Align16Bytes(w>>1);//w/2
					break;
				default:
					break;
				}
			}

			PICTURE_FORMAT(int width, int height, E_PIXFMT colorspace,
                            int _stride, int _uStride, int _vStride, int _aStride)
				: w(width), h(height), ePixfmt(colorspace)
				, stride(_stride), u_stride(_uStride), v_stride(_vStride), a_stride(_aStride)
                                , pts(0)
			{
			}

			PICTURE_FORMAT(int width, int height, E_PIXFMT colorspace, 
                            int _stride, int _uStride, int _vStride, int _aStride,
                            unsigned int _pts)
				: w(width), h(height), ePixfmt(colorspace)
				, stride(_stride), u_stride(_uStride), v_stride(_vStride), a_stride(_aStride)
                                , pts(_pts)
			{
			}

			bool isValid() const 
			{ 
				if(w<=0 || h<=0)	return false;
				switch(ePixfmt)
				{
				case PIXFMT_E_NONE:
					return false;
				case PIXFMT_E_YUY2:
				case PIXFMT_E_UYVY:
				case PIXFMT_E_RGB565:
				case PIXFMT_E_RGB24:
				case PIXFMT_E_RGB32:
				case PIXFMT_E_RGBA:
				case PIXFMT_E_BGRA:
				case PIXFMT_E_MAX:
					return y_stride>0;
				case PIXFMT_E_YV12:
				case PIXFMT_E_I420:
					return y_stride>0 && u_stride>0 && v_stride>0;
				default:
					return false;
				}
			}

			int w;
			int h;
			E_PIXFMT ePixfmt;
			union {
				int stride;
				int y_stride;
			};
			int u_stride;//仅仅平面格式用到，否则不用
			int v_stride;//仅仅平面格式用到，否则不用
			int a_stride;//仅仅平面格式用到，否则不用,when pixel format need A planer, we need this.
            uint64_t pts;//after decoded we get pts.Not need dts in PICTURE after decoded.
		};

		inline int GetBitCount(E_PIXFMT ePixfmt)
		{
			int nBitCount = 0;
			switch (ePixfmt)
			{
			case PIXFMT_E_YV12:
			case PIXFMT_E_I420:
				nBitCount = 12;
				break;
			case PIXFMT_E_YUY2:
			case PIXFMT_E_UYVY:
			case PIXFMT_E_RGB565:
				nBitCount = 16;
				break;	
			case PIXFMT_E_RGB24:
				nBitCount = 24;
				break;		
			case PIXFMT_E_RGB32:
				nBitCount = 32;
				break;
			case PIXFMT_E_RGBA:
			case PIXFMT_E_BGRA:
				nBitCount = 32;
				break;
			default:
				assert(! "GetBitCount not support this pixel format");
			}

			return nBitCount;
		}

		inline int GetPictureSize(const PICTURE_FORMAT& pH)
		{
			int nByteSize = 0;
			switch (pH.ePixfmt)
			{
			case PIXFMT_E_YV12:
			case PIXFMT_E_I420:
				nByteSize = pH.y_stride * pH.h + pH.u_stride * (pH.h >> 1) + pH.v_stride * (pH.h >> 1);
                nByteSize = Align16Bytes(nByteSize);
				break;
			case PIXFMT_E_YUY2:
			case PIXFMT_E_UYVY:
			case PIXFMT_E_RGB565:
			case PIXFMT_E_RGB24:
			case PIXFMT_E_RGB32:
			case PIXFMT_E_RGBA:
				nByteSize = pH.y_stride * pH.h;
                nByteSize = Align16Bytes(nByteSize);
				break;
			default:
				assert(! "GetBitCount不支持的格式");
			}

			return nByteSize;
		}

        // 视频数据对象
        // composite with PICTURE_FORMAT and MediaBuffer
        class PictureRaw
        {
        public:
            typedef PictureRaw SelfType;
            typedef boost::shared_ptr<PictureRaw> SPtr;

            PictureRaw();
            ~PictureRaw();

            //functions for get the information about this picture
            inline const PICTURE_FORMAT& format() const { return m_format; }
            inline const MediaBuffer& buffer() const { return m_buf; }
            inline const BYTE* data() const { return m_buf.data(); }
            inline BYTE* data() { return m_buf.data(); }
            size_t size() const	{ return m_format.isValid() ? GetPictureSize(m_format) : 0; }
            inline const BYTE* rgb() const;
            inline const BYTE* yuv() const;
            inline const BYTE* y() const;
            inline const BYTE* u() const;
            inline const BYTE* v() const;
            inline const BYTE* a() const;

            /**
             *	@name			allocData
             *	@brief			根据参数_format申请用于保存图片数据的内存空间
             *					用户可提供allocator指定内存申请以及释放使用的函数，如果未指定，则默认使用BigBufferManager中提供的共享内存管理方法
             *	@param[in]		const PICTURE_FORMAT & _format 图片的属性
             *	@param[in]		const MemoryAllocator & allocator 内存申请释放对象
             *	@return			bool true--成功  false--失败，可能_format指定的图片属性不正确，或者申请内存失败
             **/
            bool allocData(const PICTURE_FORMAT& _format, const MemoryAllocator& allocator = MemoryAllocator());
            /**
             *	@name			attachData
             *	@brief			将当前对象与已申请的内存绑定
             *					当前对象被释放时，不会释放绑定的内存，用户需要自己负责释放内存
             *					此处绑定是不坚持数据是否为空以及数据的长度是否符合_format指定的图片属性所需要的数据长度
             *					若当前对象已有数据（申请的或者绑定的）则之前的数据将不再引用，申请的将被释放，绑定的将不再绑定
             *	@param[in]		BYTE * pData 用户内存块首地址
             *	@param[in]		size_t len 内存块大小
             *	@param[in]		const PICTURE_FORMAT & _format 图片属性
             **/
            bool attachData(uint8_t* pData, size_t len, const PICTURE_FORMAT& _format, const MemoryAllocator& allocator = MemoryAllocator());

            void freeData();

            void setTimestamp(int64_t pts) { m_format.pts = pts; }
            int64_t getTimestamp() const { return m_format.pts; }

        private:
            PictureRaw(const PictureRaw& robj);
            PictureRaw& operator=(const PictureRaw& robj);

            PICTURE_FORMAT m_format;
            MediaBuffer m_buf;
        };

        const BYTE* PictureRaw::rgb() const
        {
            if(!m_format.isValid()) return NULL;
            switch (m_format.ePixfmt)
            {
            case PIXFMT_E_YV12:
            case PIXFMT_E_I420:
            case PIXFMT_E_YUY2:
            case PIXFMT_E_UYVY:
                return NULL;
            case PIXFMT_E_RGB565:
            case PIXFMT_E_RGB24:
            case PIXFMT_E_RGB32:
            case PIXFMT_E_RGBA:
            case PIXFMT_E_BGRA:
                return m_buf.data();
            default:
                assert(! "不支持的像素格式");
                return NULL;
            }
        }

        const BYTE* PictureRaw::yuv() const
        {
            if(!m_format.isValid()) return NULL;
            switch (m_format.ePixfmt)
            {
            case PIXFMT_E_YV12:
            case PIXFMT_E_I420:
            case PIXFMT_E_YUY2:
            case PIXFMT_E_UYVY:
                return m_buf.data();
            case PIXFMT_E_RGB565:
            case PIXFMT_E_RGB24:
            case PIXFMT_E_RGB32:
            case PIXFMT_E_RGBA:
            case PIXFMT_E_BGRA:
                return NULL;
            default:
                assert(! "不支持的像素格式");
                return NULL;
            }
        }

        const BYTE* PictureRaw::y() const
        {
            if(!m_format.isValid()) return NULL;
            switch (m_format.ePixfmt)
            {
            case PIXFMT_E_YV12:
            case PIXFMT_E_I420:
                return m_buf.data();
            case PIXFMT_E_YUY2:
            case PIXFMT_E_UYVY:
                return NULL;
            case PIXFMT_E_RGB565:
            case PIXFMT_E_RGB24:
            case PIXFMT_E_RGB32:
            case PIXFMT_E_RGBA:
            case PIXFMT_E_BGRA:
                return NULL;
            default:
                assert(! "不支持的像素格式");
                return NULL;
            }
        }

        const BYTE* PictureRaw::u() const
        {
            if(!m_format.isValid()) return NULL;
            switch (m_format.ePixfmt)
            {
            case PIXFMT_E_YV12:
                //yyyy vv uu
                return m_buf.data() + m_format.y_stride*m_format.h + m_format.v_stride * (m_format.h >> 1);
            case PIXFMT_E_I420:
                //yyyy uu vv
                return m_buf.data() + m_format.y_stride*m_format.h;
            case PIXFMT_E_YUY2:
            case PIXFMT_E_UYVY:
                return NULL;
            case PIXFMT_E_RGB565:
            case PIXFMT_E_RGB24:
            case PIXFMT_E_RGB32:
            case PIXFMT_E_RGBA:
            case PIXFMT_E_BGRA:
                return NULL;
            default:
                assert(! "不支持的像素格式");
                return NULL;
            }
        }

        const BYTE* PictureRaw::v() const
        {
            if(!m_format.isValid()) return NULL;
            switch (m_format.ePixfmt)
            {
            case PIXFMT_E_YV12:
                //yyyy vv uu
                return m_buf.data() + m_format.y_stride*m_format.h;
            case PIXFMT_E_I420:
                return m_buf.data() + m_format.y_stride*m_format.h + m_format.u_stride * (m_format.h >> 1);
            case PIXFMT_E_YUY2:
            case PIXFMT_E_UYVY:
                return NULL;
            case PIXFMT_E_RGB565:
            case PIXFMT_E_RGB24:
            case PIXFMT_E_RGB32:
            case PIXFMT_E_RGBA:
            case PIXFMT_E_BGRA:
                return NULL;
            default:
                assert(! "不支持的像素格式");
                return NULL;
            }
        }

        const BYTE* PictureRaw::a() const
        {
            if(!m_format.isValid()) return NULL;
            switch (m_format.ePixfmt)
            {
            case PIXFMT_E_YV12:
            case PIXFMT_E_I420:
            case PIXFMT_E_YUY2:
            case PIXFMT_E_UYVY:
            case PIXFMT_E_RGB565:
            case PIXFMT_E_RGB24:
            case PIXFMT_E_RGB32:
            case PIXFMT_E_RGBA:
            case PIXFMT_E_BGRA:
                return NULL;
            default:
                assert(! "不支持的像素格式");
                return NULL;
            }
        }

}//namespace zMedia

#endif //_MEDIA_FILTER_PICTURE_INFO_H_
