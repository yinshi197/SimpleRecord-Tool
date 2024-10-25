#pragma once

#include "Thread.h"
#include <fstream>
#include <iostream>
using namespace std;

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
}

/**
 * @brief The AudioEncodec class 封装音频编码类,父类是Thread
 */
class AudioEncodec:public Thread
{
public:
	AudioEncodec() {}
	~AudioEncodec() {}

	int Init(const char *ifile, const char *ofile,const char *codec_name);
	int Start();
	int Stop();	
	void Run();

	/* 检测该编码器是否支持该采样格式 */
	int check_sample_fmt(const AVCodec* codec, enum AVSampleFormat sample_fmt);

	/* 检测该编码器是否支持该采样率 */
	int check_sample_rate(const AVCodec* codec, const int sample_rate);

	/* 检测该编码器是否支持该通道布局, 该函数只是作参考 */
	int check_channel_layout(const AVCodec* codec, AVChannelLayout channel_layout);

    /* ADTS Heard */
	void get_adts_header();
	
	/* 编码器 */
	int encode();

	/* 这里只支持2通道的转换 */
	void f32le_convert_to_fltp(float* f32le, float* fltp, int nb_samples);

private:
	FILE* in_file_;	//输入文件句柄
	FILE* out_file_;	//输出文件句柄
	const AVCodec* codec_ = nullptr;	//编码器
	AVCodecContext* codec_ctx_ = nullptr;	//编码器上下文
	AVFrame* frame_ = nullptr;	//帧数据结构(原始数据)
	AVPacket* pkt_ = nullptr;	//编码后的数据
	int ret_ = 0;
	const char* codec_name = nullptr;	//编码器名字
	enum AVCodecID  codec_id_ = AV_CODEC_ID_AAC;	//默认AAC

	uint8_t* pcm_buf_ = nullptr;
	uint8_t* pcm_temp_buf_ = nullptr;
	uint8_t adts_header_[7];
};

