#include "widget.h"
#include "ui_widget.h"

#pragma execution_character_set(push, "utf-8")

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/image/1.jpg"));
    setWindowTitle("冷兔宝宝死忠粉录屏");

    btn = new QPushButton("录屏", this);
    connect(btn, SIGNAL(clicked()), this, SLOT(record()));

    gdi_btn = new QRadioButton("gdi模式(dxgi模式不能用可尝试选择此项)", this);
    dxgi_btn = new QRadioButton("dxgi模式(仅适用于win8,win8.1,win10,win11)", this);
    connect(gdi_btn, SIGNAL(toggled(bool)), this, SLOT(set_gdi(bool)));
    connect(dxgi_btn, SIGNAL(toggled(bool)), this, SLOT(set_dxgi(bool)));
    dxgi_btn->setChecked(true);
    mode = 1;

    time = new QLCDNumber(this);
    time->setDigitCount(12);
    time->display("00:00:00.000");
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(show_time()));

    open_dir_btn = new QPushButton("打开视频所在目录", this);
    connect(open_dir_btn, SIGNAL(clicked()), this, SLOT(open_dir()));
}

void Widget::resizeEvent(QResizeEvent*)
{
    btn->setGeometry(width() * 0.5 - 40, 10, 80, 30);
    gdi_btn->setGeometry(width() * 0.5 - 175, 90, 350, 30);
    dxgi_btn->setGeometry(width() * 0.5 - 175, 170, 350, 30);
    time->setGeometry(width() * 0.5 - 120, 250, 240, 40);
    open_dir_btn->setGeometry(width() * 0.5 - 50, 340, 100, 30);
}

void Widget::set_gdi(bool checked)
{
    if (checked)
        mode = 0;
}

void Widget::set_dxgi(bool checked)
{
    if (checked)
        mode = 1;
}

void Widget::record()
{
    if (btn->text() == "录屏")
    {
        btn->setEnabled(false);
        btn->setText("停止");
        recorder = new Recorder(mode);
        thread = new QThread;
        recorder->moveToThread(thread);
        connect(thread, SIGNAL(started()), recorder, SLOT(start()));
        connect(this, SIGNAL(stop()), recorder, SLOT(stop()));
        connect(recorder, SIGNAL(started()), this, SLOT(record_started()));
        connect(recorder, SIGNAL(save_finished()), thread, SLOT(quit()));
        connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
        connect(thread, SIGNAL(finished()), recorder, SLOT(deleteLater()));
        connect(thread, SIGNAL(finished()), this, SLOT(record_finished()));
        t = QDateTime::currentMSecsSinceEpoch();
        thread->start();
        timer->start(500);
    }
    else
    {
        btn->setEnabled(false);
        emit stop();
        timer->stop();
    }
}

void Widget::record_started()
{
    btn->setEnabled(true);
}

void Widget::record_finished()
{
    btn->setText("录屏");
    btn->setEnabled(true);
}

void Widget::show_time()
{
    t2 = QDateTime::currentMSecsSinceEpoch() - t;
    ms = t2 % 1000;
    s = t2 / 1000;
    h = s / 3600;
    s -= h * 3600;
    m = s / 60;
    s -= m * 60;
    time->display(QString("%1:%2:%3.%4").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')).arg(ms, 3, 10, QChar('0')));
}

void Widget::open_dir()
{
    QDir dir("video");
    if (!dir.exists())
        if (!dir.mkpath(dir.absolutePath()))
            return;
    QDesktopServices::openUrl(QUrl::fromLocalFile("./video/"));
}

Widget::~Widget()
{
    delete ui;
}

#pragma execution_character_set(pop)
