#ifndef RENDERSPK_H
#define RENDERSPK_H

#include <QObject>
#include "recordinfo.h"

class RenderSpk : public QObject
{
    Q_OBJECT
public:
    RenderSpk(record_t &r, QObject *parent = nullptr);
private slots:
    void render();
signals:
    void over();
private:
    record_t &r;
};

#endif // RENDERSPK_H
