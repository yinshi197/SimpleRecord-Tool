#include "AudioEncodec.h"

/**
 * @brief AudioEncodec::Init 初始化编码器参数
 * @param ifile 输入文件
 * @param ofile 输出文件
 * @param codec_name  编码器名字，目前支持 aac libfdk_aac
 * @return success 0, failed -1
 */
int AudioEncodec::Init(const char* ifile, const char* ofile,const char* codec_name)
{
	if (codec_name)
	{
		cout << "codec_name: " << codec_name << endl;
		codec_ = avcodec_find_encoder_by_name(codec_name);
	}
	else
	{
		cout << "usw defulat codec: AAC" << endl;
		codec_ = avcodec_find_encoder(codec_id_);
	}

	if (!codec_)
	{
		cout << "codec_ alloc failed" << endl;
		return -1;
	}


	//初始化编码器上下文
	codec_ctx_ = avcodec_alloc_context3(codec_);
	if (!codec_ctx_)
	{
		cout << "codec_ctx_ alloc failed" << endl;
		return -1;
	}

	//codec_ctx_->codec_id = codec_id_;
	codec_ctx_->codec_type = AVMEDIA_TYPE_AUDIO;
	codec_ctx_->bit_rate = 64 * 1024;
	codec_ctx_->ch_layout = AV_CHANNEL_LAYOUT_STEREO;
	codec_ctx_->sample_rate = 48000;
	codec_ctx_->profile = FF_PROFILE_AAC_HE_V2;		//三种模式 aac_low 128kb , aac_he 64kb  , aac_he_v2
	if (strcmp(codec_->name, "aac") == 0)
	{
		codec_ctx_->sample_fmt = AV_SAMPLE_FMT_FLTP;
	}
	else if (strcmp(codec_->name, "libfdk_aac") == 0)
	{
		codec_ctx_->sample_fmt = AV_SAMPLE_FMT_S16;
	}
	else
	{
		codec_ctx_->sample_fmt = AV_SAMPLE_FMT_S16;
	}

	if (!check_sample_fmt(codec_, codec_ctx_->sample_fmt))
	{
		cout << "Encoder does not support sample format " << av_get_sample_fmt_name(codec_ctx_->sample_fmt) << endl;
		return -1;
	}

	if (!check_sample_rate(codec_, codec_ctx_->sample_rate))
	{
		cout << "Encoder does not support sample rate " << codec_ctx_->sample_rate << endl;
		return -1;
	}

	if (!check_channel_layout(codec_, codec_ctx_->ch_layout))
	{
		char buff[64] = { 0 };
		//输出channel_layout的名字(或者说是名字描述)
		av_channel_layout_describe(&codec_ctx_->ch_layout, buff, 64);
		cout << "Encoder does not support channel layout " << buff << endl;
		return -1;
	}

	char buff[64] = { 0 };
	av_channel_layout_describe(&codec_ctx_->ch_layout, buff, 64);
	cout << "\n" << "---------Audio Encodec Configuration Parameters---------" << endl;
	cout << "bit_rate: " << codec_ctx_->bit_rate / 1024 << "kbps" << endl;
	cout << "sample_rate: " << codec_ctx_->sample_rate << endl;
	cout << "sample_fmt: " << av_get_sample_fmt_name(codec_ctx_->sample_fmt) << endl;
	cout << "channle_layout: " << buff << endl;
	cout << "channles: " << codec_ctx_->ch_layout.nb_channels << endl;
	cout << "(befor avcodec_open2)frame size: " << codec_ctx_->frame_size << endl;

	//Place global headers in extradata instead of every keyframe
	//设置标记flags为AV_CODEC_FLAG_GLOBAL_HEADER表示：将全局标头放在额外数据中，而不是每个关键帧中
    if (strcmp(codec_->name, "aac") == 0)
    {
        codec_ctx_->flags = AV_CODEC_FLAG_GLOBAL_HEADER;    //aac需要使用adts才能正常播放，而fdk_aac不带adts也能播放
    }

	//将编码器和编码器上下文进行关连
	if (avcodec_open2(codec_ctx_, codec_, nullptr) < 0)
	{
		cout << "avcodec_open2 failed" << endl;
		return -1;
	}
	
	//使用avcodec_open2会自动计算frame_size(每个通道的样本数)
	cout << "(after avcodec_open2)frame size: " << codec_ctx_->frame_size << endl;

	in_file_ = fopen(ifile, "rb");
	if (!in_file_)
	{
		cout << "open file " << ifile << "failed" << endl;
		return -1;
	}

	out_file_ = fopen(ofile, "wb");
	if (!out_file_)
	{
		cout << "open file " << ofile << "failed" << endl;
		return -1;
	}

	pkt_ = av_packet_alloc();
	if (!pkt_)
	{
		cout << "av_packet_alloc failed" << endl;
		return -1;
	}

	frame_ = av_frame_alloc();
	if (!frame_)
	{
		cout << "av_frame_alloc failed" << endl;
		return -1;
 	}

	/* 每次送多少数据给编码器由：
	*  (1)frame_size(每帧单个通道的采样点数);
	*  (2)sample_fmt(采样点格式);
	*  (3)channel_layout(通道布局情况);
	*  3个要素决定
	*/

	frame_->nb_samples = codec_ctx_->frame_size;
	frame_->format = codec_ctx_->sample_fmt;
	frame_->ch_layout = codec_ctx_->ch_layout;

	//为frame分配空间,0表示不进行内存对齐，设置好上面的三个参数就可以自动分配frame
	ret_ = av_frame_get_buffer(frame_, 0);
	if (ret_ < 0)
	{
		cout << "av_frame_get_buffer failed" << endl;
        return -1;
	}

    cout << "AudioEncodec Init Finish.\n";
	return 0;
}

/**
 * @brief AudioEncodec::Start 启动函数，内部创建线程调用Run函数
 * @return success 0, failed -1
 */
int AudioEncodec::Start()
{
	thread_ = new std::thread(&AudioEncodec::Run, this);
	if (!thread_)
	{
		cout << "new thread failed" << endl;
		return -1;
	}

	return 0;
}

/**
 * @brief AudioEncodec::Run 编码运行函数
 */
void AudioEncodec::Run()
{
	//计算每帧数据的大小 单个采样点的字节(format) * 通道数量(channel) * 每帧采样点数量(nb_sample)
	int frame_bytes = av_get_bytes_per_sample((AVSampleFormat)frame_->format) \
		* frame_->ch_layout.nb_channels \
		* frame_->nb_samples;
	cout << "frame_bytes size: " << frame_bytes << endl;

	pcm_buf_ = (uint8_t*)malloc(frame_bytes);	//堆分配的内存，需要手动释放
	if (!pcm_buf_)
	{
		cout << "pcm_buf alloc failed" << endl;
		exit(-1);
	}

	pcm_temp_buf_ = (uint8_t*)malloc(frame_bytes);	//堆分配的内存，需要手动释放
	if (!pcm_temp_buf_)
	{
		cout << "pcm_temp_buf alloc failed" << endl;
		exit(-1);
	}

	int64_t pts = 0;
	cout << "\n----------Start Encode----------" << endl;
	for (;;)
	{
		memset(pcm_buf_, 0, frame_bytes);	//填充pcm_buf内存为全0
		size_t read_bytes = fread(pcm_buf_, 1, frame_bytes, in_file_);	//读取一帧数据
        //cout << "read file date size " << read_bytes << endl;
		if (read_bytes <= 0)	//一定要写 =,因为后面有一个冲刷的过程，会有等于0的可能。
		{
			cout << "read file finish\n";
			break;
		}

		/* 确保该frame可写, 如果编码器内部保持了内存参考计数，则需要重新拷贝一个备份
		   目的是新写入的数据和编码器保存的数据不能产生冲突
	    */
		ret_ = av_frame_make_writable(frame_);
		if (ret_ != 0)
		{
			cout << "av_frame_make_writable failed" << endl;
			exit(-1);
		}

		if (frame_->format == AV_SAMPLE_FMT_S16)
		{
			// 将读取到的PCM数据填充到frame去，但要注意格式的匹配, 是planar还是packed都要区分清楚
			ret_ = av_samples_fill_arrays(frame_->data,
										  frame_->linesize,
										  pcm_buf_, 
				                          frame_->ch_layout.nb_channels,
				                          frame_->nb_samples,
				                          (AVSampleFormat)frame_->format,
				                          0);
		}
		else
		{
			// 将读取到的PCM数据填充到frame去，但要注意格式的匹配, 是planar还是packed都要区分清楚
			// 将本地的f32le packed模式的数据转为float palanar
			memset(pcm_temp_buf_, 0, frame_bytes);
			f32le_convert_to_fltp((float*)pcm_buf_, (float*)pcm_temp_buf_, frame_->nb_samples);
			ret_ = av_samples_fill_arrays(frame_->data, 
										  frame_->linesize,
										  pcm_temp_buf_, 
										  frame_->ch_layout.nb_channels,
										  frame_->nb_samples, 
										  (AVSampleFormat)frame_->format, 
										  0);
		}

		//设置pts
		pts += frame_->nb_samples;
		frame_->pts = pts;	// 使用采样率作为pts的单位，具体换算成秒 pts*1/采样率
        cout << "Audio PTS: " << frame_->pts << endl;
		ret_ = encode();	//每帧调用一次
		if (ret_ < 0)
		{
			cout << "encode is failed" << endl;
			break;
		}
	}
}

/**
 * @brief AudioEncodec::Stop 结束函数，内部调用父类Thread的Stop函数，
 * @return 0
 */
int AudioEncodec::Stop()
{
    Thread::Stop();

    /* 冲刷编码器 */
    encode();

    // 关闭文件
    if(in_file_)
    {
        fclose(in_file_);
    }

    if(out_file_)
    {
        fclose(out_file_);
    }


    // 释放内存
    if (pcm_buf_)
    {
        free(pcm_buf_);
    }

    if (pcm_temp_buf_)
    {
        free(pcm_temp_buf_);
    }

    if(frame_)
    {
        av_frame_free(&frame_);
    }

    if(pkt_)
    {
        av_packet_free(&pkt_);
    }

    if(codec_ctx_)
    {
        avcodec_free_context(&codec_ctx_);
    }

    cout << "AudioEncodec Stop Finish.\n";
    return 0;
}

/**
 * @brief AudioEncodec::check_sample_fmt 检测该编码器是否支持该采样格式
 * @param codec 编码器
 * @param sample_fmt 采样格式
 * @return success 1, failed 0
 */
int AudioEncodec::check_sample_fmt(const AVCodec* codec, AVSampleFormat sample_fmt)
{
    //获取编码器的sample_fmt
    const AVSampleFormat* p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE)	//AV_SAMPLE_FMT_NONE作为结束符
    {
        if (*p == sample_fmt)
            return 1;
        p++;
    }

    return 0;
}

/**
 * @brief AudioEncodec::check_sample_rate 检测该编码器是否支持该采样率
 * @param codec 编码器
 * @param sample_rate 采样率
 * @return success 1, failed 0
 */
int AudioEncodec::check_sample_rate(const AVCodec* codec, const int sample_rate)
{
	const int* p = codec->supported_samplerates;
	while (*p != 0)
	{
		cout << codec->name << " support " << *p << "hz" << endl;
		if (*p == sample_rate)
			return 1;
		p++;
	}
	return 0;
}

/**
 * @brief AudioEncodec::check_channel_layout 检测该编码器是否支持该通道布局, 该函数只是作参考
 * @param codec 编码器
 * @param channel_layout 声道布局
 * @return success 1, failed 0
 */
int AudioEncodec::check_channel_layout(const AVCodec* codec, AVChannelLayout channel_layout)
{

	//不是每个codec都给出了支持的channle_layout
	const AVChannelLayout* p = codec->ch_layouts;

	if (!p)
	{
		cout << "the codec" << codec->name << " no set channel_layouts" << endl;

		if (av_channel_layout_check(&channel_layout))
		{
			return 1;
		}
		else return 0;

	}
	else
	{
		//可以利用 nb_channels 字段来判断是否到达数组的终止标记，因为零填充的 AVChannelLayout 结构体应该有 nb_channels 字段为零
		while (p->nb_channels != 0)
		{
			if (av_channel_layout_compare(p, &channel_layout) == 0)
				return 1;
			p++;
		}

	}

	return 0;
}

/**
 * @brief AudioEncodec::get_adts_header ADTS Heard 写ADTS头部信息
 */
void AudioEncodec::get_adts_header()
{
	uint8_t freq_idx = 0;    //0: 96000 Hz  3: 48000 Hz 4: 44100 Hz
	switch (codec_ctx_->sample_rate) {
	case 96000: freq_idx = 0; break;
	case 88200: freq_idx = 1; break;
	case 64000: freq_idx = 2; break;
	case 48000: freq_idx = 3; break;
	case 44100: freq_idx = 4; break;
	case 32000: freq_idx = 5; break;
	case 24000: freq_idx = 6; break;
	case 22050: freq_idx = 7; break;
	case 16000: freq_idx = 8; break;
	case 12000: freq_idx = 9; break;
	case 11025: freq_idx = 10; break;
	case 8000: freq_idx = 11; break;
	case 7350: freq_idx = 12; break;
	default: freq_idx = 4; break;
	}
	uint8_t chanCfg = codec_ctx_->ch_layout.nb_channels;
	uint32_t frame_length = pkt_->size + 7;
	adts_header_[0] = 0xFF;
	adts_header_[1] = 0xF1;
	adts_header_[2] = ((codec_ctx_->profile) << 6) + (freq_idx << 2) + (chanCfg >> 2);
	adts_header_[3] = (((chanCfg & 3) << 6) + (frame_length >> 11));
	adts_header_[4] = ((frame_length & 0x7FF) >> 3);
	adts_header_[5] = (((frame_length & 7) << 5) + 0x1F);
	adts_header_[6] = 0xFC;
}

/**
 * @brief AudioEncodec::encode 音频编码器
 * @return success 0, failed -1
 */
int AudioEncodec::encode()
{
	int ret = 0;
	ret = avcodec_send_frame(codec_ctx_, frame_);
	if (ret < 0)
	{
		cout << "avcodec_send_frame failed" << endl;
		return -1;
	}

	/* read all the available output packets (in general there may be any number of them */
	// 编码和解码都是一样的，都是send 1次，然后receive多次, 直到AVERROR(EAGAIN)或者AVERROR_EOF
	while (ret >= 0)
	{
		//获取编码好的音频包，如果成功则需要重复获取，直到失败
		ret = avcodec_receive_packet(codec_ctx_, pkt_);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)	//AVERROR(EAGAIN)数据为空，需要获取更多数据    AVERROR_EOF编码结束
		{
			return 0;	//break
		}
		else if (ret < 0)		//出错
		{
			cout << "Error encoding audio frame\n";
			return -1;
		}

		size_t len = 0;
		//printf("ctx->flags:0x%x & AV_CODEC_FLAG_GLOBAL_HEADER:0x%x, name:%s\n", codec_ctx_->flags, codec_ctx_->flags & AV_CODEC_FLAG_GLOBAL_HEADER, ctx->codec->name);
		if ((codec_ctx_->flags & AV_CODEC_FLAG_GLOBAL_HEADER)) //为0不需要， 为1（flags不是AV_CODEC_FLAG_GLOBAL_HEADER）需要在每帧数据加上ADTS Header
		{
			get_adts_header();
			len = fwrite(adts_header_, 1, 7, out_file_);
			if (len != 7) 
			{
				cout << "fwrite aac_header failed\n";
				return -1;
			}
		}

		len = fwrite(pkt_->data, 1, pkt_->size, out_file_);
		if (len != pkt_->size)
		{
			cout << "fwrite aac data failed" << endl;
			return -1;
		}
		/* 是否需要释放数据? avcodec_receive_packet第一个调用的就是 av_packet_unref
		 * 所以我们不用手动去释放，这里有个问题，不能将pkt直接插入到队列，因为编码器会释放数据
		 * 可以新分配一个pkt, 然后使用av_packet_move_ref转移pkt对应的buffer
		 */
		// av_packet_unref(pkt);
	}

	return 0;
}

/**
 * @brief AudioEncodec::f32le_convert_to_fltp Planar(平面)格式转换为Packet(交错)格式
 * @param f32le f32le(源)数据buff
 * @param fltp  fltp(目标)数据buff
 * @param nb_samples 源格式样本大小
 */
void AudioEncodec::f32le_convert_to_fltp(float* f32le, float* fltp, int nb_samples)
{
	float* fltp_l = fltp;	//左通道
	float* fltp_r = fltp + nb_samples;	//右通道
	for (int i = 0; i < nb_samples; i++)
	{
		fltp_l[i] = f32le[i * 2];	// 0(左) 1(右)  ----  2 3
		fltp_r[i] = f32le[i * 2 + 1]; // 可以尝试注释单个声道
	}
}


