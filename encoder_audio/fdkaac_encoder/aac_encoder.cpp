#include "aac_encoder.h"
#include <stdlib.h>
#include <stdio.h>
#include <vcruntime_string.h>

static const char* fdkaac_error(AACENC_ERROR erraac)
{
    switch (erraac)
    {
    case AACENC_OK: return "No error";
    case AACENC_INVALID_HANDLE: return "Invalid handle";
    case AACENC_MEMORY_ERROR: return "Memory allocation error";
    case AACENC_UNSUPPORTED_PARAMETER: return "Unsupported parameter";
    case AACENC_INVALID_CONFIG: return "Invalid config";
    case AACENC_INIT_ERROR: return "Initialization error";
    case AACENC_INIT_AAC_ERROR: return "AAC library initialization error";
    case AACENC_INIT_SBR_ERROR: return "SBR library initialization error";
    case AACENC_INIT_TP_ERROR: return "Transport library initialization error";
    case AACENC_INIT_META_ERROR: return "Metadata library initialization error";
    case AACENC_ENCODE_ERROR: return "Encoding error";
    case AACENC_ENCODE_EOF: return "End of file";
    default: return "Unknown error";
    }
}

AACEncoder* aac_encoder_init(const int sample_rate, const int channels, const int bit_rate, const int profile_aac)
{
    AACENC_ERROR ret;
    AACEncoder* aac_enc_handle_ = NULL;
    aac_enc_handle_ = (AACEncoder*)malloc(sizeof(AACEncoder));
    if (!aac_enc_handle_)
    {
        printf("Can't malloc memory\n");
        return NULL;
    }
    memset(aac_enc_handle_, 0, sizeof(AACEncoder));

    // 打开编码器,如果非常需要节省内存则可以调整encModules
    if ((ret = aacEncOpen(&aac_enc_handle_->handle, 0x0, channels)) != AACENC_OK)
    {
        printf("Unable to open fdkaac encoder, ret = 0x%x, error is %s\n", ret, fdkaac_error(ret));
        free(aac_enc_handle_);
        return NULL;
    }

    // 设置AAC标准格式
    if ((ret = aacEncoder_SetParam(aac_enc_handle_->handle, AACENC_AOT, profile_aac)) != AACENC_OK) /* aac lc */
    {
        printf("Unable to set the AACENC_AOT, ret = 0x%x, error is %s\n", ret, fdkaac_error(ret));
        goto faild;
    }
    // 设置采样率
    if ((ret = aacEncoder_SetParam(aac_enc_handle_->handle, AACENC_SAMPLERATE, sample_rate)) != AACENC_OK)
    {
        printf("Unable to set the SAMPLERATE, ret = 0x%x, error is %s\n", ret, fdkaac_error(ret));
        goto faild;
    }
    // 设置通道数
    if ((ret = aacEncoder_SetParam(aac_enc_handle_->handle, AACENC_CHANNELMODE, channels)) != AACENC_OK)
    {
        printf("Unable to set the AACENC_CHANNELMODE, ret = 0x%x, error is %s\n", ret, fdkaac_error(ret));
        goto faild;
    }
    // 设置比特率
    if ((ret = aacEncoder_SetParam(aac_enc_handle_->handle, AACENC_BITRATE, bit_rate)) != AACENC_OK)
    {
        printf("Unable to set the AACENC_BITRATE, ret = 0x%x, error is %s\n", ret, fdkaac_error(ret));
        goto faild;
    }
    // 设置编码出来的数据带aac adts头
    if ((ret = aacEncoder_SetParam(aac_enc_handle_->handle, AACENC_TRANSMUX, TT_MP4_ADTS)) != AACENC_OK) // 0-raw 2-adts
    {
        printf("Unable to set the ADTS AACENC_TRANSMUX, ret = 0x%x, error is %s\n", ret, fdkaac_error(ret));
        goto faild;
    }
    // 初始化编码器
    if ((ret = aacEncEncode(aac_enc_handle_->handle, NULL, NULL, NULL, NULL)) != AACENC_OK)
    {
        printf("Unable to initialize the aacEncEncode, ret = 0x%x, error is %s\n", ret, fdkaac_error(ret));
        goto faild;
    }
    // 获取编码器信息
    if ((ret = aacEncInfo(aac_enc_handle_->handle, &aac_enc_handle_->info)) != AACENC_OK)
    {
        printf("Unable to get the aacEncInfo info, ret = 0x%x, error is %s\n", ret, fdkaac_error(ret));
        goto faild;
    }

    // 计算pcm帧长
    aac_enc_handle_->pcm_frame_len = aac_enc_handle_->info.inputChannels * aac_enc_handle_->info.frameLength * 2;

    printf("AACEncoderInit   channels = %d, pcm_frame_len = %d\n",
        aac_enc_handle_->info.inputChannels, aac_enc_handle_->pcm_frame_len);

    return  aac_enc_handle_;
faild:
    if (aac_enc_handle_ && aac_enc_handle_->handle)
    {
        if (aacEncClose(&aac_enc_handle_->handle) != AACENC_OK)
        {
            printf("aacEncClose failed\n");
        }
    }
    if (aac_enc_handle_)
        free(aac_enc_handle_);

    return NULL;
}

int aac_encoder_encode(AACEncoder* handle, const int8_t* input, const int input_len, int8_t* output, int* output_len)
{
    AACENC_ERROR ret;

    if (!handle)
    {
        return AACENC_INVALID_HANDLE;
    }

    if (input_len != handle->pcm_frame_len)
    {
        printf("input_len = %d no equal to need length = %d\n", input_len, handle->pcm_frame_len);
        return AACENC_UNSUPPORTED_PARAMETER;            // 每次都按帧长的数据进行编码
    }

    AACENC_BufDesc  out_buf = { 0 };
    AACENC_InArgs   in_args = { 0 };

    // pcm数据输入配置
    in_args.numInSamples = input_len / 2; // 所有通道的加起来的采样点数，每个采样点是2个字节所以/2

    // pcm数据输入配置
    int     in_identifier = IN_AUDIO_DATA;
    int     in_elem_size = 2;
    void* in_ptr = (int8_t*)input;
    int     in_buffer_size = input_len;
    AACENC_BufDesc  in_buf = { 0 };
    in_buf.numBufs = 1;
    in_buf.bufs = &in_ptr;
    in_buf.bufferIdentifiers = &in_identifier;
    in_buf.bufSizes = &in_buffer_size;
    in_buf.bufElSizes = &in_elem_size;

    // 编码数据输出配置
    int out_identifier = OUT_BITSTREAM_DATA;
    int out_elem_size = 1;
    void* out_ptr = output;
    int out_buffer_size = *output_len;
    out_buf.numBufs = 1;
    out_buf.bufs = &out_ptr;
    out_buf.bufferIdentifiers = &out_identifier;
    out_buf.bufSizes = &out_buffer_size;        //一定要可以接收解码后的数据
    out_buf.bufElSizes = &out_elem_size;

    AACENC_OutArgs  out_args = { 0 };

    if ((ret = aacEncEncode(handle->handle, &in_buf, &out_buf, &in_args, &out_args)) != AACENC_OK)
    {
        printf("aacEncEncode ret = 0x%x, error is %s\n", ret, fdkaac_error(ret));

        return ret;
    }
    *output_len = out_args.numOutBytes;

    return AACENC_OK;
}

void aac_encoder_deinit(AACEncoder** handle)
{
    // 先关闭编码器
    if (aacEncClose(&(*handle)->handle) != AACENC_OK)
    {
        printf("aacEncClose failed\n");
    }
    // 释放内存
    free(*handle);
    // 将handle指向NULL
    *handle = NULL;
}

/**
* 添加ADTS头部 这里只是为了以后rtmp拉流时存储aac文件做准备
*
* @param packet    ADTS header 的 byte[]，长度为7
* @param packetLen 该帧的长度，包括header的长度
* @param profile  0-Main profile, 1-AAC LC，2-SSR
* @param freqIdx    采样率
                    0: 96000 Hz
                    1: 88200 Hz
                    2: 64000 Hz
                    3: 48000 Hz
                    4: 44100 Hz
                    5: 32000 Hz
                    6: 24000 Hz
                    7: 22050 Hz
                    8: 16000 Hz
                    9: 12000 Hz
                    10: 11025 Hz
                    11: 8000 Hz
                    12: 7350 Hz
                    13: Reserved
                    14: Reserved
                    15: frequency is written explictly
* @param chanCfg    通道
                    2：L+R
                    3：C+L+R
*/
void add_adts_to_packet(int8_t* packet, int packetLen, int profile, int freqIdx, int chanCfg)
{
    /*
    int profile = 2; // AAC LC
    int freqIdx = 3; // 48000Hz
    int chanCfg = 2; // 2 Channel
    */
    packet[0] = 0xFF;
    packet[1] = 0xF9;
    packet[2] = (((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
    packet[3] = (((chanCfg & 3) << 6) + (packetLen >> 11));
    packet[4] = ((packetLen & 0x7FF) >> 3);
    packet[5] = (((packetLen & 7) << 5) + 0x1F);
    packet[6] = 0xFC;
}