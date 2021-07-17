#ifndef RECORDVIDEO_H
#define RECORDVIDEO_H

#include <QObject>
#include "recordinfo.h"

class RecordVideo : public QObject
{
    Q_OBJECT
public:
    RecordVideo(record_t &r, queue_t &q, QObject *parent = nullptr);
private slots:
    void record_gdi();
    void record_dxgi();
private:
    record_t &r;
    queue_t &q;

    AVFrame *frame;
    AVFrame *frame_ref;
    AVFrame *frame_tmp;
};

#endif // RECORDVIDEO_H
