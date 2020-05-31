#ifndef _AAC_ENCODER_H_
#define _AAC_ENCODER_H_

#include <stdint.h>
#include <aacenc_lib.h>
#include <FDK_audio.h>
#ifdef __cplusplus
extern "C"
{
#endif
    // 对应
#define PROFILE_AAC_LC      2               // AOT_AAC_LC
#define PROFILE_AAC_HE      5               // AOT_SBR
#define PROFILE_AAC_HE_v2   29              // AOT_PS PS, Parametric Stereo (includes SBR)  
#define PROFILE_AAC_LD      23              // AOT_ER_AAC_LD Error Resilient(ER) AAC LowDelay object
#define PROFILE_AAC_ELD     39              // AOT_ER_AAC_ELD AAC Enhanced Low Delay

    typedef struct aac_encoder
    {
        HANDLE_AACENCODER handle;   // fdk-aac
        AACENC_InfoStruct info;     // fdk-aac
        int pcm_frame_len;          // 每次送pcm的字节数
    }AACEncoder;

    /*
    支持的采样率sample_rate：8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000
    */
    AACEncoder* aac_encoder_init(const int sample_rate, const int channels, const int bit_rate, const int profile_aac);
    int aac_encoder_encode(AACEncoder* handle, const int8_t* input, const int input_len, int8_t* output, int* output_len);
    void aac_encoder_deinit(AACEncoder** handle);

#ifdef __cplusplus
}
#endif
#endif // !__AAC_ENCODER_H__