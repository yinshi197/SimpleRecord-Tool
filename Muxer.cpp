#include "Muxer.h"

int Muxer::Init(char *ofilename, char *audio_filename, char *video_filename)
{
    av_log_set_level(AV_LOG_ERROR);

    ofilename_ = ofilename;
    audio_filename_ = audio_filename;
    video_filename_ = video_filename;

    ret_ = avformat_alloc_output_context2(&fmt_ctx_, nullptr, nullptr, ofilename_);
    if(ret_ < 0)
    {
        av_strerror(ret_, error, sizeof(error - 1));
        cout << "Muxer::Init avformat_alloc_output_context2 to fialed, " << error << endl;
        return ret_;
    }

    oformat_ = fmt_ctx_->oformat;

    ret_ = OpenVideoStream();
    ret_ = OpenAudioStream();

}

int Muxer::Start()
{
    thread_ = new std::thread(&Muxer::Run, this);
    if (!thread_)
    {
        cout << "new thread failed" << endl;
        return -1;
    }
    return 0;
}

int Muxer::Stop()
{
    Thread::Stop2();
    if(fmt_ctx_video_)
    {
        avformat_close_input(&fmt_ctx_video_);
    }

    if(fmt_ctx_audio_)
    {
        avformat_close_input(&fmt_ctx_audio_);
    }

    if(oformat_ && !(fmt_ctx_->flags & AVFMT_NOFILE))
    {
        avio_close(fmt_ctx_->pb);
    }

    if(fmt_ctx_)
    {
        avformat_free_context(fmt_ctx_);
    }

    return 0;
}

void Muxer::Run()
{
    /* open the output file, if needed */
    if (!(oformat_->flags & AVFMT_NOFILE))
    {
        ret_ = avio_open(&fmt_ctx_->pb, ofilename_, AVIO_FLAG_WRITE);
        if(ret_ < 0)
        {
            av_strerror(ret_, error, sizeof(error) - 1);
            cout << "Muxer::Run avio_open to failed:" << error << endl;
            return;
        }
    }

    //写Header
    ret_ = avformat_write_header(fmt_ctx_, nullptr);
    if(ret_ < 0)
    {
        av_strerror(ret_, error, sizeof(error - 1));
        cout << "Muxer::Run avformat_write_header to fialed, " << error << endl;
        return;
    }
    // 循环写入视频和音频帧
    while (writing_v || writing_a)
    {
        // 如果还在写视频帧，且（不写音频帧或者视频帧的PTS小于等于音频帧的PTS）
        if (writing_v &&
            (!writing_a ||
             av_compare_ts(cur_pts_v, fmt_ctx_video_->streams[videoindex_in]->time_base, cur_pts_a,
                           fmt_ctx_audio_->streams[audioindex_in]->time_base) <= 0))
        {
            // 读取并写入视频帧
            ret_ = WriteVideo();
            if (ret_ == -2)
            {
                writing_v = 0;
            }
            else if (ret_ < 0)
            {
                return;
            }
        }
        else
        {   // 读取并写入音频帧
            ret_ = WriteAudio();
            if (ret_ == -2)
            {
                writing_a = 0;
            }
            else if (ret_ < 0)
            {
                return;
            }
        }
    }

    //写入文件
    ret_ = av_write_trailer(fmt_ctx_);
    if(ret_ < 0)
    {
        av_strerror(ret_, error, sizeof(error) - 1);
        cout << "Muxer::Run av_write_trailer to failed:" << error << endl;
    }

}

int Muxer:: OpenAudioStream()
{
    AVStream *out_stream;

    if(!audio_filename_)
    {
        cout <<"audio_filename_ is nullptr" << endl;
        return -1;
    }

    ret_ = avformat_open_input(&fmt_ctx_audio_, audio_filename_, nullptr, nullptr);
    if(ret_ < 0)
    {
        av_strerror(ret_, error, sizeof(error) - 1);
        cout << "Muxer::OpenAudioStream avformat_open_input :" << error << endl;
        return ret_;
    }

    avformat_find_stream_info(fmt_ctx_audio_, nullptr);

    audioindex_in = av_find_best_stream(fmt_ctx_audio_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if(audioindex_in < 0)
    {
        cout << "find audio index to failed" << endl;
        return -1;
    }
    cout << "Audio index = " << audioindex_in << endl;

    out_stream = avformat_new_stream(fmt_ctx_, nullptr);
    if(!out_stream)
    {
        cout << "Muxer:: OpenAudioStream out_stream is nullptr" << endl;
        return -1;
    }

    audioindex_out = out_stream->index;

    ret_ = avcodec_parameters_copy(out_stream->codecpar, fmt_ctx_audio_->streams[audioindex_in]->codecpar);
    if(ret_ < 0)
    {
        av_strerror(ret_, error, sizeof(error) - 1);
        cout << "Muxer::OpenAudioStream avcodec_parameters_copy to failed :" << error << endl;
        return ret_;
    }


    // out_stream->codecpar->codec_tag = 0;
    // //是否需要写ADTS头
    // if (fmt_ctx_->oformat->flags & AVFMT_GLOBALHEADER)
    //     fmt_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    return 0;
}

int Muxer::OpenVideoStream()
{
    AVStream *out_stream;

    if(!video_filename_)
    {
        cout <<"video_filename_ is nullptr" << endl;
    }
    ret_ = avformat_open_input(&fmt_ctx_video_, video_filename_, nullptr, nullptr);
    if(ret_ < 0)
    {
        av_strerror(ret_, error, sizeof(error) - 1);
        cout << "Muxer::OpenVideoStream avformat_open_input :" << error << endl;
        return ret_;
    }

    avformat_find_stream_info(fmt_ctx_video_, nullptr);

    videoindex_in = av_find_best_stream(fmt_ctx_video_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if(videoindex_in < 0)
    {
        cout << "find video index to failed" << endl;
    }

    cout << "Video index = " << videoindex_in << endl;


    out_stream = avformat_new_stream(fmt_ctx_, nullptr);
    if(!out_stream)
    {
        cout << "Muxer:: OpenAudioStream out_stream is nullptr" << endl;
        return -1;
    }

    videoindex_out = out_stream->index;

    ret_ = avcodec_parameters_copy(out_stream->codecpar, fmt_ctx_video_->streams[videoindex_in]->codecpar);
    if(ret_ < 0)
    {
        av_strerror(ret_, error, sizeof(error) - 1);
        cout << "Muxer::OpenAudioStream avcodec_parameters_copy to failed :" << error << endl;
        return ret_;
    }

    out_stream->codecpar->codec_tag = 0;

    return 0;
}

int Muxer::WriteAudio()
{
    AVPacket pkt;
    AVStream *in_stream, *out_stream;

    if((ret_ = av_read_frame(fmt_ctx_audio_, &pkt)) < 0)
    {
        if (ret_ == AVERROR_EOF) // 读到文件尾
        {
            return -2;
        }
        else
        {
            cout << "av_read_frame failed" << endl;
            return -1;
        }
    }

    in_stream = fmt_ctx_audio_->streams[pkt.stream_index];
    out_stream = fmt_ctx_->streams[audioindex_out];

    cur_pts_v = pkt.pts;

    // 转换PTS和DTS
    pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base,
                               (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base,
                               (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
    pkt.pos = -1;
    pkt.stream_index = audioindex_out;

    cout << "Write 1 audio Packet. size:" << pkt.size << " ,pts:" << pkt.pts << endl;

    // 写入帧
    if ((ret_ = av_interleaved_write_frame(fmt_ctx_, &pkt)) < 0)
    {
        cout << "av_interleaved_write_frame failed\n";
        av_packet_unref(&pkt);
        return -1;
    }

    av_packet_unref(&pkt);
    return 0;

}

int Muxer::WriteVideo()
{
    AVPacket pkt;
    AVStream *in_stream, *out_stream;

    if((ret_ = av_read_frame(fmt_ctx_video_, &pkt)) < 0)
    {
        if (ret_ == AVERROR_EOF) // 读到文件尾
        {
            return -2;
        }
        else
        {
            cout << "av_read_frame failed" << endl;
            return -1;
        }
    }

    in_stream = fmt_ctx_video_->streams[pkt.stream_index];
    out_stream = fmt_ctx_->streams[videoindex_out];

    // 如果PTS值无效，计算并设置PTS和DTS
    if (pkt.pts == AV_NOPTS_VALUE)
    {
        AVRational time_base = in_stream->time_base;
        int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate) * 2;
        pkt.pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base) * AV_TIME_BASE);
        pkt.dts = pkt.pts;
        pkt.duration = (double)calc_duration / (double)(av_q2d(time_base) * AV_TIME_BASE);
        frame_index++;
    }
    cur_pts_v = pkt.pts;

    // 转换PTS和DTS
    pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base,
                               (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base,
                               (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
    pkt.pos = -1;
    pkt.stream_index = videoindex_out;

    cout << "Write 1 video Packet. size:" << pkt.size << " ,pts:" << pkt.pts << endl;

    // 写入帧
    if ((ret_ = av_interleaved_write_frame(fmt_ctx_, &pkt)) < 0)
    {
        cout << "av_interleaved_write_frame failed\n";
        av_packet_unref(&pkt);
        return -1;
    }

    av_packet_unref(&pkt);
    return 0;
}
