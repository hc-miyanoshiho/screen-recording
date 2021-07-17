#ifndef MERGEAUDIO_H
#define MERGEAUDIO_H

#include <QObject>
#include <QDateTime>
#include "recordinfo.h"

class MixAudio : public QObject
{
    Q_OBJECT
public:
    MixAudio(record_t &r, QObject *parent = nullptr);
private slots:
    void mix();
signals:
    void write_finished();
private:
    record_t &r;
    AVFrame *frame;
    AVFrame *frameAAC;
    AVPacket *packetAAC;
};

#endif // MERGEAUDIO_H
