#include "RecordAudio.h"

/**
 * @brief RecordAudio::Init 初始化相关参数
 * @return success 0, failed -1
 */
int RecordAudio::Init()
{
    //1.注册设备
    avdevice_register_all();

    ifmt_ = av_find_input_format("dshow");
    if (!ifmt_)
    {
        cout << "av_find_input_format failed" << endl;
        return -1;
    }

    av_dict_set(&opts_, "loglevel", "debug", 0);
    av_dict_set(&opts_, "channels", "2", 0);
    av_dict_set(&opts_, "sample_rate", "48000", 0);     //
    av_dict_set(&opts_, "sample_size", "16", 0);

    int ret = avformat_open_input(&fmt_ctx_, "audio=麦克风 (Realtek(R) Audio)", ifmt_, &opts_);
    if (ret < 0)
    {
        cout << "avformat_open_input failed" << endl;
        return -1;
    }

    swr_ctx_ = swr_alloc();
    if (!swr_ctx_)
    {
        cout << "swr_alloc failed" << endl;
        return -1;
    }

    ret = swr_alloc_set_opts2(&swr_ctx_,
        &dst_layout_,
        dst_fmt_,
        dst_rate_,
        &src_layout_,
        src_fmt_,
        src_rate_,
        0,
        nullptr
    );
    if (ret < 0)
    {
        cout << "swr_alloc_set_opts2 failed" << endl;
        return -1;
    }

    ret = swr_init(swr_ctx_);
    if (ret < 0)
    {
        cout << "swr_init failed" << endl;
        return -1;
    }

    out_ = fopen("./test.pcm", "wb");       //重采样后的数据
    if(!out_)
    {
        cout << "open file failed\n";
        return -1;
    }

    out2_ = fopen("./test2.pcm", "ab");     //重采样后的数据，不过是追加写入
    if(!out2_)
    {
        cout << "open file2 failed\n";
        return -1;
    }

    out3_ = fopen("./test3.pcm", "wb");     //重采样前的数据
    if(!out3_)
    {
        cout << "open file3 failed\n";
        return -1;
    }

    pkt_ = av_packet_alloc();
    if(!pkt_)
    {
        cout << "av_packet_alloc failed\n";
        return -1;
    }

    cout << "RecordAudio Init 完成" << endl;

    return 0;
}

/**
 * @brief RecordAudio::Start 启动函数
 * @return success 0, failed -1
 */
int RecordAudio::Start()
{
    thread_ = new std::thread(&RecordAudio::Run, this);
    if (!thread_)
    {
        cout << "new thread failed" << endl;
        return -1;
    }
    return 0;
}

/**
 * @brief RecordAudio::Stop 结束线程函数
 * @return 0
 */
int RecordAudio::Stop()
{
    Thread::Stop();

    //先关闭输入设备，防止继续采集数据。
    if (fmt_ctx_)
    {
        avformat_close_input(&fmt_ctx_);
    }

    if (out_)
    {
        fclose(out_);
    }

    if (out2_)
    {
        fclose(out2_);
    }

    if (out3_)
    {
        fclose(out3_);
    }

    if (swr_ctx_)
    {
        swr_close(swr_ctx_);
    }

    if (pkt_)
    {
        av_packet_free(&pkt_);
    }

    cout << "Recordaudio Stop" << endl;
    return 0;
}

/**
 * @brief RecordAudio::Run 运行函数
 */
void RecordAudio::Run()
{
    cout << "开始录音" << endl;
    while (av_read_frame(fmt_ctx_, pkt_) == 0 && abort_ == 0)
    {
        int nb_sample_size = pkt_->size / (2 * src_layout_.nb_channels);
        //cout << "pkt.size = " << pkt_->size << endl;
        av_samples_alloc_array_and_samples(&src_data_,
            &src_linesize_,
            src_layout_.nb_channels,
            nb_sample_size,
            src_fmt_,
            0);

        av_samples_alloc_array_and_samples(&dst_data_,
            &dst_linesize_,
            dst_layout_.nb_channels,
            nb_sample_size,
            dst_fmt_,
            0);

        memcpy((void*)src_data_[0], (void*)pkt_->data, pkt_->size);
        int ret = swr_convert(swr_ctx_, dst_data_, nb_sample_size, src_data_, nb_sample_size);
        //cout << "重采样数据大小:" << ret << endl;
        fwrite(dst_data_[0], 1, dst_linesize_, out_);
        fwrite(dst_data_[0], 1, dst_linesize_, out2_);
        fwrite(pkt_->data, 1, pkt_->size, out3_);
        fflush(out_);
        fflush(out2_);
        fflush(out3_);
        av_packet_unref(pkt_);
    }

}


