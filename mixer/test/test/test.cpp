// test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "zAudioMixer.h"

FILE* inputPcmFile = nullptr;
FILE* inputPcmFile_2 = nullptr;
FILE* outputFile = nullptr;

int readSamples(unsigned char* buf, int buflen, int sampleCount, int sampleSize, FILE* pcmFile)
{
    if (nullptr == pcmFile)
        return 0;
    int len = sampleCount * sampleSize;
    len = len <= buflen ? len : buflen;
    size_t readed = fread_s(buf, len, 1, len, pcmFile);
    return readed;
}

int main()
{
    errno_t err = fopen_s(&inputPcmFile, "rec_send-talk.pcm", "rb");
    if (inputPcmFile == nullptr)
    {
        printf("can not open pcm file for input\n");
        return -1;
    }
    err = fopen_s(&inputPcmFile_2, "rec_play-all-0.pcm", "rb");
    if (inputPcmFile_2 == nullptr)
    {
        printf("can not open pcm file for input\n");
        return -2;
    }
    err = fopen_s(&outputFile, "mixed.pcm", "wb+");
    if (outputFile == nullptr)
    {
        printf("can not open pcm file for output\n");
        return -3;
    }
    int sampleCount = 3840;
    int sampleSize = sizeof(short);
    int buflen = sampleCount * sampleSize;
    unsigned char* buf = (unsigned char*)malloc(buflen);
    unsigned char* buf_2 = (unsigned char*)malloc(buflen);

    std::vector<short*> srcDataVec;
    std::vector<double> srcVolumeVec;
    double outputVolumeFactor = 1.0f;

    srcDataVec.push_back((short*)buf);
    srcDataVec.push_back((short*)buf_2);
    srcVolumeVec.push_back(1.0f);
    srcVolumeVec.push_back(1.0f);

    unsigned char* buf_mixed = (unsigned char*)malloc(buflen);
    while (0!=readSamples(buf, buflen, sampleCount, sampleSize, inputPcmFile)
        && 0!=readSamples(buf_2, buflen, sampleCount, sampleSize, inputPcmFile_2))
    {
        mixPcms(srcDataVec, sampleCount, srcVolumeVec, (short*)buf_mixed, outputVolumeFactor);
        fwrite(buf_mixed, 1, buflen, outputFile);
    }

    //int sampleCount = 3840;
    //short* outputPcmBuf = (short*)malloc(sizeof(short) * 2 * sampleCount);
    std::cout << "Hello World!\n";
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
