#include "RecordVideo.h"

/**
 * @brief RecordVideo::Init 初始化相关参数
 * @return success 0, failed -1
 */
int RecordVideo::Init()
{
    //1.注册设备
    avdevice_register_all();

    ifmt_ = av_find_input_format("dshow");
    if (!ifmt_)
    {
        cout << "av_find_input_format failed" << endl;
        return -1;
    }

    //ffmpeg -f dshow -list_options true -i video="Chicony USB2.0 Camera"
    //设置采集参数，目前存在BUG 视频相关参数只能在QT MinGW编译才能有效，且video_size固定是1920x1080更改了也不生效

    //video="Chicony USB2.0 Camera"
    av_dict_set(&opts_, "loglevel", "debug", 0);
    av_dict_set(&opts_, "video_size", "640x480", 0);
    av_dict_set(&opts_, "framerate", "30", 0);     //
    av_dict_set(&opts_, "pixel_format", "yuyv422", 0);

    //video="screen-capture-recorder"
    // av_dict_set(&opts_, "pixel_format", "yuv420p", 0);
    // av_dict_set(&opts_, "video_size", "1920x1080", 0);
    // av_dict_set(&opts_, "framerate", "25", 0);

    //video="screen-capture-recorder"   Chicony USB2.0 Camera
    int ret = avformat_open_input(&fmt_ctx_, "video=Chicony USB2.0 Camera", ifmt_, &opts_);
    if (ret < 0)
    {
        cout << "avformat_open_input failed" << endl;
        av_strerror(ret, err_buf_, 64);
        cout << "error info: " << err_buf_ << endl; 
        return -1;
    }

    out_ = fopen("./test2.yuv", "wb");   //编码前数据
    out2_ = fopen("./test.h264", "wb"); //编码后的数据
    pkt_ = av_packet_alloc();
    pkt2_ = av_packet_alloc();
    frame_ = av_frame_alloc();

    //SDL相关
    win_width_ = 640;
    win_height_ = 480;
    window_ = SDL_CreateWindow("Screen Recording",
                               SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               win_width_, win_height_,
                               SDL_WINDOW_OPENGL); //SDL_WINDOW_RESIZABLE
    if(!window_)
    {
        cout << "Create SDL Window is failed\n";
        return -1;
    }

    render_ = SDL_CreateRenderer(window_, -1, 0);
    if(!render_)
    {
        cout << "Create SDL Render is failed\n";
        return -1;
    }

    cout << "RecordVideo Init 完成" << endl;

    return 0;
}

/**
 * @brief RecordVideo::Start
 * @return success 0, failed -1
 */
int RecordVideo::Start()
{
    thread_ = new std::thread(&RecordVideo::Run, this);
    if (!thread_)
    {
        cout << "new thread failed" << endl;
        return -1;
    }
    return 0;
}

/**
 * @brief RecordVideo::Stop
 * @return 0
 */
int RecordVideo::Stop()
{
    Thread::Stop();

    //先关闭输入设备，防止继续采集数据。
    if (fmt_ctx_)
    {
        avformat_close_input(&fmt_ctx_);
    }

    if(frame_)
    {
        av_frame_free(&frame_);
        Encodec();  //最后再冲刷一次，编码最后的数据。否则会缺帧
    }

    if (out_)
    {
        fclose(out_);
    }

    if (out2_)
    {
        fclose(out2_);
    }

    if (pkt_)
    {
        av_packet_free(&pkt_);
    }

    if(display_flag_)
    {
        if(window_)
        {
            SDL_DestroyWindow(window_);
        }

        if(render_)
        {
            SDL_DestroyRenderer(render_);
        }

        if(texture_)
        {
            SDL_DestroyTexture(texture_);
        }

        SDL_Quit();
    }


    cout << "RecordVideo Stop" << endl;
    return 0;
}

/**
 * @brief RecordVideo::Run 开始录屏
 * @param 无
 * @return 无
 */
void RecordVideo::Run()
{
    int base = 0;
    cout << "开始录屏" << endl;

    OpenEncodec(640,480);

    video_width_ = 640;
    video_height_ = 480;

    texture_ = SDL_CreateTexture(render_, SDL_PIXELFORMAT_YUY2,   //纹理格式
                                SDL_TEXTUREACCESS_STREAMING,  //纹理访问模式 mode:变化频繁，可锁定
                                video_width_, video_height_);
    if(!texture_)
    {
        cout << "Create SDL Window is failed\n";
    }

    frame_->width = video_width_;
    frame_->height = video_height_;
    frame_->format = AV_PIX_FMT_YUV420P;
    frame_->pts = 0;
    int ret = av_frame_get_buffer(frame_, 32);      //视频按32位对齐
    if(ret < 0)
    {
        av_strerror(ret, err_buf_, 64);
        cout << "Could not av_frame_get_buffer,error onfo :" << err_buf_ << endl;
        exit(-1);
    }

    sws_ctx_ = sws_getContext(video_width_, video_height_, AV_PIX_FMT_YUYV422,
                             video_width_, video_height_, AV_PIX_FMT_YUV420P,
                             SWS_BILINEAR, nullptr, nullptr, nullptr);


    while (av_read_frame(fmt_ctx_, pkt_) == 0 && abort_ == 0)
    {

        cout << "pkt.size = " << pkt_->size << endl;
        av_frame_make_writable(frame_);
        const uint8_t* srcSlice[] = { pkt_->data };
        int srcStride[] = { video_width_ * 2 }; // YUYV422 每行字节数 = 宽度 * 2
        sws_scale(sws_ctx_, srcSlice, srcStride, 0,
                  video_height_, frame_->data, frame_->linesize);
        // memcpy(frame_->data[0], pkt_->data, 1920*1080);     //Y
        // memcpy(frame_->data[1], pkt_->data+1920*1080, 1920*1080/4);     //U
        // memcpy(frame_->data[2], pkt_->data+1920*1080+1920*1080/4, 1920*1080/4);     //V
        fwrite(pkt_->data, 1, pkt_->size , out_);
        fflush(out_);
        frame_->pts = base++;
        cout << "Video PTS: " << frame_->pts << endl;
        Encodec();
        av_packet_unref(pkt_);

        if(display_flag_)
        {
            SDL_PollEvent(&event_);
            switch(event_.type)
            {
                case SDL_QUIT: SDL_DestroyWindow(window_);break;
                case SDL_WINDOWEVENT:
                {
                    // SDL_GetWindowSize(window_, &win_width_, &win_height_);
                    // SDL_Log("SDL_WINDOWEVENT win_width:%d, win_height:%d\n", win_width_,win_height_);
                    break;
                }
                default:break;
            }
        }
    }
}

/**
 * @brief RecordVideo::OpenEncodec 打开编码，初始化相关参数 默认使用libx264
 * @param width 宽
 * @param height 高
 */
void RecordVideo::OpenEncodec(int width, int height)
{
    int ret = 0;
    codec_ = avcodec_find_encoder_by_name("libx264");
    if(!codec_)
    {
        cout << "Could not alloc codec_" << endl;
        exit(-1);
    }

    codec_ctx_ = avcodec_alloc_context3(codec_);
    if(!codec_ctx_)
    {
        cout << "Could not alloc codec_ctx_" << endl;
    }

    //设置编码器参数(SPS和PPS)
    codec_ctx_->profile = FF_PROFILE_H264_HIGH_444;
    codec_ctx_->level = 50;         //表示level是5.0

    //设置分辨率
    codec_ctx_->width = width;
    codec_ctx_->height = height;

    //设置GOP
    codec_ctx_->gop_size = 50;
    codec_ctx_->keyint_min = 25;    //最小gop大小

    //设置B帧
    codec_ctx_->max_b_frames = 3;
    codec_ctx_->has_b_frames = 1;

    //设置参考帧数量
    codec_ctx_->refs = 3;

    //设置输入yuv格式
    codec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P; //AV_PIX_FMT_YUYV422

    //设置码率
    codec_ctx_->bit_rate = 1200 * 1024; //2500kbps

    //设置帧率
    codec_ctx_->time_base = av_make_q(1,25);
    codec_ctx_->framerate = av_make_q(25,1);

    ret = avcodec_open2(codec_ctx_, codec_, nullptr);
    if(ret < 0)
    {
        av_strerror(ret, err_buf_, 64);
        cout << "Could not open codec_,error onfo :" << err_buf_ << endl;
        exit(-1);
    }
}

/**
 * @brief RecordVideo::Encodec 视频编码器
 */
void RecordVideo::Encodec()
{
    //渲染图片
    Render();
    int ret = avcodec_send_frame(codec_ctx_, frame_);
    if(ret < 0)
    {
        av_strerror(ret, err_buf_, 64);
        cout << "Could not avcodec_send_frame,error onfo :" << err_buf_ << endl;
        exit(-1);
    }

    while(ret >= 0)
    {
        ret = avcodec_receive_packet(codec_ctx_, pkt2_);
        if(ret == AVERROR(EAGAIN) || ret ==AVERROR_EOF )
        {
            return;
        }
        else if(ret < 0)
        {
            av_strerror(ret, err_buf_, 64);
            cout << "receive_packet error,error onfo :" << err_buf_ << endl;
            exit(-1);
        }

        cout << "Pkt2 PTS:" << pkt2_->pts << endl;
        fwrite(pkt2_->data, 1, pkt2_->size, out2_);
        fflush(out2_);
        av_packet_unref(pkt2_);
    }
}

void RecordVideo::Render()
{
    if(pkt_)
    {
        rect_.x = 0;
        rect_.y = 0;
        float w_ract = win_width_ * 1.0 / video_width_;
        float h_ract = win_height_ * 1.0 / video_height_;
        rect_.w = video_width_ * w_ract;
        rect_.h = video_height_ * h_ract;

        //第二个参数nullptr 更新整个纹理，设置为&rect将更新rect区域的纹理
        // SDL_UpdateYUVTexture(texture_, nullptr,
        //                      frame2_->data[0], frame2_->linesize[0],
        //                      frame2_->data[1], frame2_->linesize[1],
        //                      frame2_->data[2], frame2_->linesize[2]);
        SDL_UpdateTexture(texture_, nullptr, pkt_->data, video_width_ * 2); //AV_PIX_FMT_YUYV422格式是两个字节一个像素点
        SDL_RenderClear(render_);

        SDL_RenderCopy(render_, texture_, nullptr, &rect_);

        SDL_RenderPresent(render_);
        cout << "__________________Render_______________\n";
    }

}

void RecordVideo::SDL_Stop()
{
    display_flag_ = 0;

    if(window_)
    {
        SDL_DestroyWindow(window_);
    }

    if(render_)
    {
        SDL_DestroyRenderer(render_);
    }

    if(texture_)
    {
        SDL_DestroyTexture(texture_);
    }

    SDL_Quit();
}


