#ifndef RECORDAUDIO_H
#define RECORDAUDIO_H
#include "Thread.h"
#include <iostream>
using namespace std;

extern "C"
{
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
}

/**
 * @brief The RecordAudio class 封装音频采集类，父类是Thread
 */
class RecordAudio :public Thread
{
public:
    RecordAudio() {}
    ~RecordAudio() {}
    int Init();
    int Start();
    int Stop();
    void Run();
private:
    const AVInputFormat* ifmt_ = nullptr;
    AVFormatContext* fmt_ctx_ = nullptr;
    FILE* out_ = nullptr;
    FILE* out2_ = nullptr;  //追加写入重采样数据
    FILE* out3_ = nullptr;  //存储s16格式的数据
    AVPacket* pkt_ = nullptr;

    //重采样输入参数
    SwrContext* swr_ctx_ = nullptr;     //重采样上下文
    AVChannelLayout src_layout_ = AV_CHANNEL_LAYOUT_STEREO;
    AVSampleFormat src_fmt_ = AV_SAMPLE_FMT_S16;
    int  src_rate_ = 48000;
    uint8_t** src_data_ = nullptr;
    int src_linesize_;

    //重采样输出参数
    AVChannelLayout dst_layout_ = AV_CHANNEL_LAYOUT_STEREO;
    AVSampleFormat dst_fmt_ = AV_SAMPLE_FMT_FLT;
    int  dst_rate_ = 48000;
    uint8_t** dst_data_ = nullptr;
    int dst_linesize_;

    AVDictionary *opts_ = nullptr;
};

#endif // RECORDAUDIO_H
