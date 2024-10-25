#ifndef RecordVideo_H
#define RecordVideo_H
#include "Thread.h"
#include <iostream>
#include <SDL.h>
using namespace std;

extern "C"
{
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
}

/**
 * @brief The RecordVideo class 视频的原始数据比较大，封装视频采集和编码类默认编码为h264。父类Thread
 */
class RecordVideo :public Thread
{
public:
    RecordVideo(){}
    ~RecordVideo(){}
    int Init();
    int Start();
    int Stop();
    void Run();
    void OpenEncodec(int width, int hight);
    void Encodec();
    void Render();
    void SDL_Stop();
private:
    const AVInputFormat* ifmt_ = nullptr;
    AVFormatContext* fmt_ctx_ = nullptr;
    AVCodecContext* codec_ctx_ = nullptr;
    const AVCodec* codec_ = nullptr;  //编码器
    FILE* out_ = nullptr;   //原始数据
    FILE* out2_ = nullptr;  //编码数据
    AVPacket* pkt_ = nullptr;
    AVPacket* pkt2_ = nullptr;
    AVFrame* frame_ = nullptr;
    AVDictionary* opts_ = nullptr;

    char err_buf_[64] = { 0 };  //错误信息
    struct SwsContext* sws_ctx_ = nullptr;

    //SDL相关
    int display_flag_ = 1;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* render_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    SDL_Event event_;
    SDL_Rect rect_;
    int win_width_;
    int win_height_;
    int video_width_;
    int video_height_;
};

#endif // RecordVideo_H

