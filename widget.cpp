#include "widget.h"
#include "ui_widget.h"
#include "signalhandler.h"
#include "fcntl.h"
#include <QThread>
#include <QDebug>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    //创建多线程,模拟测试多线程环境
    thread1 = new QThread(this);
    thread1->start();
    thread2 = new QThread(this);
    thread2->start();
    thread3 = new QThread(this);
    thread3->start();
    qDebug()<<"sub thread:"<<thread1<<thread2<<thread3;

    connect(SignalHandler::getInstance(),&SignalHandler::signalReceivedSig,
            this,&Widget::signalReceivedSlot);
}

Widget::~Widget()
{
    delete ui;
    thread1->quit();
    thread1->wait();
    thread2->quit();
    thread2->wait();
    thread3->quit();
    thread3->wait();
}

//注册信号处理
void Widget::on_registerSIgBtn_clicked()
{
    int sigNum = ui->sigNumSpinBox->value();
    QStringList saMaskStrList = ui->saMaskLineEdit->text().split(",");
    QList<int> saMaskList;
    for(int i=0;i<saMaskStrList.size();i++)
    {
        saMaskList.append(saMaskStrList.at(i).toInt());
    }

    SignalHandler::getInstance()->registerSignal(sigNum,saMaskList);
}
//初始化fasync设备
void Widget::on_initFasyncDeviceBtn_clicked()
{
    if(fasync_fd != -1)
    {
        ::close(fasync_fd);
        fasync_fd = -1;
    }
    fasync_fd = SignalHandler::getInstance()->openFasyncIODevice(ui->fasyncDeviceLineEdit->text());
    if(fasync_fd != -1)
    {
        ui->textBrowser->append("Open fasync device success.");
    }
    else
    {
        ui->textBrowser->append("Open fasync device failed.");
    }
}
//关闭fasync设备
void Widget::on_closeFasyncDeviceBtn_clicked()
{
    if(fasync_fd != -1)
    {
        ::close(fasync_fd);
        fasync_fd = -1;
    }
}
//忽略信号
void Widget::on_ignoreSigBtn_clicked()
{
    int sigNum = ui->sigNumSpinBox->value();
    QStringList saMaskStrList = ui->saMaskLineEdit->text().split(",");
    QList<int> saMaskList;
    for(int i=0;i<saMaskStrList.size();i++)
    {
        saMaskList.append(saMaskStrList.at(i).toInt());
    }
    SignalHandler::getInstance()->registerSignal(sigNum,saMaskList,true);
}
//清空内容
void Widget::on_clearContentBtn_clicked()
{
    ui->textBrowser->clear();
}
//信号接收处理
void Widget::signalReceivedSlot(int sigNum, int sigVal)
{
    QString str = QString("Received Signal:num=%1  val=%2").arg(sigNum).arg(sigVal);
    ui->textBrowser->append(str);
    if(sigNum == SIGIO && sigVal == fasync_fd)
    {
        char buf[1];
        int ret_size = ::read(fasync_fd,&buf,sizeof(buf));
        if(ret_size == sizeof(buf))
        {
            QString str = QString("Read fasync_fd data=%1").arg((int)buf[0]);
            ui->textBrowser->append(str);
        }
    }
}
