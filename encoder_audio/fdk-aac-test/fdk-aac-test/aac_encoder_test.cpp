#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "aac_encoder.h"

#define PCM_FILE_NAME  "buweishui_48000_2_s16le.pcm"
#define AAC_FILE_NAME  "buweishui_48000_2_s16le_AAC_LC_128k.aac"

/**
* @brief get_millisecond
* @return 返回毫秒
*/
int64_t get_current_time_msec()
{
#ifdef _WIN32
    return (int64_t)GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((int64_t)tv.tv_sec * 1000 + (unsigned long long)tv.tv_usec / 1000);
#endif
}

int get_file_len(const char* file_nmae)
{
    FILE* file;
    long size;

    file = fopen(file_nmae, "rb");
    if (file == NULL)
        perror("Error opening file");
    else
    {
        fseek(file, 0, SEEK_END);    //将文件指针移动文件结尾
        size = ftell(file);             ///求出当前文件指针距离文件开始的字节数
        fclose(file);
    }
    return (int)size;
}

int main_aac_encoder_test(void)
{
    printf("hello aac\n");

    AACEncoder* aac_enc_handle = NULL;
    aac_enc_handle = aac_encoder_init(48000, MODE_2, 128000, PROFILE_AAC_LC);
    if (!aac_enc_handle)
    {
        printf("aac_encoder_init failed");
        system("pause");
        exit(-1);
    }

    // pcm buffer缓冲区, 送s16 交错格式 LRLRLR
    int     pcm_len = aac_enc_handle->pcm_frame_len;
    int8_t* pcm_buf = (int8_t*)malloc(pcm_len);    // XXX个采样点，2字节一个采样点，2通道
    int     read_len = 0;   // 每次读取回来的数据长度

    // 编码后的数据
    int     frame_max_len = 1024 * 8;        // 2的13次方
    int8_t* frame_buf = (int8_t*)malloc(frame_max_len);
    int     frame_len = frame_max_len;

    // 统计编码进度（百分比）
    int read_total_len = 0;
    int read_count = 0;
    float enc_percent = 0.0;
    int file_total_len = get_file_len(PCM_FILE_NAME);

    if (file_total_len <= 0)
    {
        printf("get_file_len failed\n");
        system("pause");
        return -1;
    }

    // 打开一个pcm文件，读取知道结束
    FILE* file_pcm = fopen(PCM_FILE_NAME, "rb");
    FILE* file_aac = fopen(AAC_FILE_NAME, "wb+");

    // 计算编码时间
    int64_t start_time = get_current_time_msec();
    int64_t cur_time = 0;
    while (feof(file_pcm) == 0)
    {
        // PCM格式要满足 S16 LRLR...LRLR 的格式
        read_len = fread(pcm_buf, 1, pcm_len, file_pcm);
        read_total_len += read_len;

        if (read_len <= 0)
        {
            printf("fread failed\n");
            break;
        }
        if (read_len < pcm_len)
        {
            printf("剩余数据不足一帧，用0补足一帧数据\n");
            memset(pcm_buf + read_len, 0, pcm_len - read_len);
        }
        // 编码
        int temp_len = 0;
        frame_len = frame_max_len;          // frame_len在输入的时候代表着frame_buf的最大存储空间
        int ret = aac_encoder_encode(aac_enc_handle, pcm_buf, pcm_len, frame_buf, &frame_len);
        if (ret != 0)
        {
            printf("aac_encoder_encode failed, ret = 0x%x\n", ret);
            break;
        }
        // 写入文件
        fwrite(frame_buf, 1, frame_len, file_aac);
        if (frame_len > temp_len)
            temp_len = frame_len;

        if (++read_count % 100 == 0)
        {
            cur_time = get_current_time_msec();
            enc_percent = (float)(1.0 * read_total_len / file_total_len * 100);
            printf("encode progress = %0.2f%%, elapsed time = %lld, temp_len = %d\n",
                enc_percent, cur_time - start_time, temp_len);
        }
        /*
        if (read_total_len > file_total_len / 2)
        {
            break;
        }
        */
    }
    cur_time = get_current_time_msec();
    printf("encode finish, elapsed time = %lld\n", cur_time - start_time);
    if (file_aac)
        fclose(file_aac);
    if (file_pcm)
        fclose(file_pcm);
    aac_encoder_deinit(&aac_enc_handle);
    free(frame_buf);
    free(pcm_buf);

    system("pause");
    return 0;
}