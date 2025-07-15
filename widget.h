#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_registerSIgBtn_clicked();
    void on_initFasyncDeviceBtn_clicked();
    void on_closeFasyncDeviceBtn_clicked();
    void on_ignoreSigBtn_clicked();
    void on_clearContentBtn_clicked();

    void signalReceivedSlot(int sigNum,int sigVal);

private:
    Ui::Widget *ui;

    int fasync_fd = -1;
    QThread *thread1 = nullptr;
    QThread *thread2 = nullptr;
    QThread *thread3 = nullptr;

};

#endif // WIDGET_H
