#ifndef RECORDER_H
#define RECORDER_H

#include <QObject>
#include "renderspk.h"
#include "mixaudio.h"
#include "recordvideo.h"
#include "writevideo.h"
#include <QThread>
#include <QFile>
#include <QDir>

class Recorder : public QObject
{
    Q_OBJECT
public:
    Recorder(int mode, QObject *parent = nullptr);
    int initGDIVideoInput();
    int initDXGIVideoInput();
    int initVideoOutput();
    int initMicInput();
    int initSpkRender();
    int initSpkInput();
    int initAudioOutput();
    int initAudioFilter();
    int initVideoFilter();
    void record();
    void save();
private slots:
    void start();
    void stop();
    void finished();
signals:
    void started();
    void save_finished();
private:
    int mode;
    record_t r;
    queue_t q;
    RenderSpk *render_spk;
    QThread *render_thread;
    MixAudio *mix_audio;
    QThread *mix_thread;
    RecordVideo *record_video;
    QThread *video_thread;
    WriteVideo *write_video;
    QThread *write_thread;

    QString video_filename;
    QString audio_filename;
    QString filename;
};

#endif // RECORDER_H
