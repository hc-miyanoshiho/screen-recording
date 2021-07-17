#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QRadioButton>
#include "recorder.h"
#include <QLCDNumber>
#include <QTimer>
#include <QDesktopServices>
#include <QUrl>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
protected:
    void resizeEvent(QResizeEvent*);
private slots:
    void set_gdi(bool);
    void set_dxgi(bool);
    void record();
    void record_started();
    void record_finished();
    void show_time();
    void open_dir();
signals:
    void stop();
private:
    Ui::Widget *ui;
    QPushButton *btn;
    QRadioButton *gdi_btn;
    QRadioButton *dxgi_btn;
    QLCDNumber *time;
    QPushButton *open_dir_btn;
    Recorder *recorder;
    QThread *thread;
    QTimer *timer;
    qint64 t, t2;
    qint64 h, m, s, ms;
    int mode;
};
#endif // WIDGET_H
