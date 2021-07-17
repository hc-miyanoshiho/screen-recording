QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimedia

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mixaudio.cpp \
    recorder.cpp \
    recordvideo.cpp \
    renderspk.cpp \
    widget.cpp \
    writevideo.cpp

HEADERS += \
    mixaudio.h \
    recorder.h \
    recordinfo.h \
    recordvideo.h \
    renderspk.h \
    widget.h \
    writevideo.h

FORMS += \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += D:/ffmpeg-20200831-4a11a6f-win64-dev/include

LIBS += -LD:/ffmpeg-20200831-4a11a6f-win64-dev/lib -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale

#MSVC
#LIBS += -lDxgi -lD3D11 -luser32 -lole32 -lAvrt

#MinGW
LIBS += -ld3d11 -lole32 -lksuser

#QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"

RESOURCES += \
    desktop_icon.rc \
    my_res.qrc

DISTFILES += \
    desktop_icon.rc

RC_FILE += desktop_icon.rc
