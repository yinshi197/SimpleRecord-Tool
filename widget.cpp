#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setWindowTitle("Record Test");
    this->buttonflag_ = true;
}

Widget::~Widget()
{
    delete ui;
}

/**
 * @brief Widget::Strat 启动
 * @return success 0, failed -1
 */
int Widget::Strat()
{
    recordaudio_ = new RecordAudio();
    recordvideo_ = new RecordVideo();

    // 初始化 RecordAudio 对象
    if (recordaudio_->Init() != 0) {
        std::cerr << "Failed to initialize audio recorder." << std::endl;
        return -1;
    }

    // 初始化 RecordVideo 对象
    if (recordvideo_->Init() != 0) {
        std::cerr << "Failed to initialize video recorder." << std::endl;
        return -1;
    }


    // 启动音频录制线程
    if (recordaudio_->Start() != 0) {
        std::cerr << "Failed to start audio recorder thread." << std::endl;
        return -1;
    }

    // 启动视频录制线程
    if (recordvideo_->Start() != 0) {
        std::cerr << "Failed to start audio recorder thread." << std::endl;
        return -1;
    }

    return 0;
}

/**
 * @brief Widget::Stop 停止
 * @return success 0, failed -1
 */
int Widget::Stop()
{
    // 停止音频录制线程
    if (recordaudio_->Stop() != 0) {
        std::cerr << "Failed to stop audio recorder thread." << std::endl;
        return -1;
    }
    std::cout << "Audio recording stopped." << std::endl;

    // 停止视频录制线程
    if (recordvideo_->Stop() != 0) {
        std::cerr << "Failed to stop video recorder thread." << std::endl;
        return -1;
    }
    std::cout << "Video recording stopped." << std::endl;

    //开始音频编码线程
    audioencodec_ = new AudioEncodec();

    if (audioencodec_->Init("test3.pcm", "test.aac", "libfdk_aac") != 0)
    {
        std::cerr << "Failed to init audio encoder thread." << std::endl;
        return -1;
    }

    if (audioencodec_->Start() != 0)
    {
        std::cerr << "Failed to start audio encoder thread." << std::endl;
        return -1;
    }

    if (audioencodec_->Stop() != 0)
    {
        std::cerr << "Failed to stop audio encoder thread." << std::endl;
        return -1;
    }
    std::cout << "Audio encodecing stopped." << std::endl;

    muxer_ = new Muxer();
    muxer_->Init("test.flv", "test.aac", "test.h264");
    cout << "muxer_ Init()" << endl;

    muxer_->Run();
    cout << "muxer_ Run()" << endl;

    muxer_->Stop();
    cout << "muxer_ Stop() OK" << endl;

    return 0;
}

void Widget::on_pushButton_clicked()
{
    if(buttonflag_)
    {
        if(this->Strat() != 0)
        {
            this->Stop();
            exit(-1);
        }
        else
        {
            ui->pushButton->setText("停止录制");
            buttonflag_ = false;
        }
    }
    else
    {
        if(this->Stop() != 0)
        {
            exit(-1);
        }
        else
        {
            ui->pushButton->setText("开始录制");
            buttonflag_ = true;
        }

    }

}

