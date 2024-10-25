QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    AudioEncodec.cpp \
    Muxer.cpp \
    RecordAudio.cpp \
    RecordVideo.cpp \
    main.cpp \
    widget.cpp

HEADERS += \
    AudioEncodec.h \
    Muxer.h \
    RecordAudio.h \
    RecordVideo.h \
    Thread.h \
    widget.h

FORMS += \
    widget.ui

INCLUDEPATH += $$PWD/ffmpeg-7.0-fdk_aac/include
INCLUDEPATH += $$PWD/SDL2-2.0.10/include
LIBS += $$PWD/ffmpeg-7.0-fdk_aac/bin/av*.dll
LIBS += $$PWD/ffmpeg-7.0-fdk_aac/bin/sw*.dll
LIBS += $$PWD/ffmpeg-7.0-fdk_aac/bin/po*.dll
LIBS += $$PWD/ffmpeg-7.0-fdk_aac/bin/Additional/li*.dll
LIBS += $$PWD/ffmpeg-7.0-fdk_aac/bin/Additional/z*.dll
LIBS += $$PWD/SDL2-2.0.10/lib/x64/SDL2.dll

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
