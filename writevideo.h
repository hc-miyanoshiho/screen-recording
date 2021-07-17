#ifndef WRITEVIDEO_H
#define WRITEVIDEO_H

#include <QObject>
#include "recordinfo.h"

class WriteVideo : public QObject
{
    Q_OBJECT
public:
    WriteVideo(record_t &r, queue_t &q, int mode, QObject *parent = nullptr);
private slots:
    void write();
signals:
    void write_finished();
private:
    record_t &r;
    queue_t &q;

    AVFrame *frame;
    AVFrame *frameYUV;
    AVPacket *packetYUV;

    int mode;
};

#endif // WRITEVIDEO_H
