#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "AudioEncodec.h"
#include "RecordAudio.h"
#include "RecordVideo.h"
#include "Muxer.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    int Strat();
    int Stop();

private slots:
    void on_pushButton_clicked();

private:
    Ui::Widget *ui;
    RecordAudio* recordaudio_ = nullptr;
    RecordVideo* recordvideo_ = nullptr;
    AudioEncodec* audioencodec_ = nullptr;
    Muxer* muxer_ = nullptr;
    bool buttonflag_;

};
#endif // WIDGET_H
