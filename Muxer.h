#ifndef MUXER_H
#define MUXER_H
#include "Thread.h"
#include <iostream>
using namespace std;

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class Muxer :public Thread
{
public:
    Muxer() {}
    ~Muxer() {}
    int Init(char *ofilename, char *audio_filename, char *video_filename);
    int Start();
    int Stop();
    void Run();

    int OpenAudioStream();
    int OpenVideoStream();

    int WriteAudio();
    int WriteVideo();

private:
    const char *ofilename_ = nullptr;
    const char *audio_filename_ = nullptr;
    const char *video_filename_ = nullptr;

    int videoindex_in = -1, videoindex_out = -1;
    int audioindex_in = -1, audioindex_out = -1;

    int frame_index = 0;
    int64_t cur_pts_v = 0, cur_pts_a = 0;
    int writing_v = 1, writing_a = 1;

    AVFormatContext *fmt_ctx_ = nullptr;
    AVFormatContext *fmt_ctx_audio_ = nullptr;
    AVFormatContext *fmt_ctx_video_ = nullptr;

    const AVOutputFormat *oformat_ = nullptr;

    char error[128];
    int ret_ = -1;
    AVDictionary *opt_ = nullptr;
};

#endif // MUXER_H
