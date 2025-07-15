/*
 *@author: 缪庆瑞
 *@date:   2019.6.24
 *@update: 2025.7.13
 *@brief:  负责Linux信号的响应处理
 *
 * 该类主要用来响应处理Linux的signal信号(与Qt的信号槽没有关系)，信号可以来源于内核态的驱动程序(一般使用固定的信号值SIGIO,即
 * fasync异步通知)或者由其他进程(IPC，可以自定义SIGRTMIN--SIGRTMAX信号的功能)发出。
 *
 * 信号安装使用sigaction()函数，支持信号带参数。信号的处理函数执行类似于软中断，在异步通知时为了确保处理函数的安全性，内部采
 * 用自管道方案，通过管道传递信号通知，具体的处理由Qt的QSocketNotifier借助事件循环执行,最终通过Qt的信号槽机制发射信号。
 *
 */
#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <signal.h>
#include <unistd.h>
#include <QObject>
#include <QSocketNotifier>

class SignalHandler : public QObject
{
    Q_OBJECT
public:
    ~SignalHandler();
    //单例模式
    static SignalHandler * getInstance()
    {
        static SignalHandler signalHandler;
        return &signalHandler;
    }
    //信号安装注册
    void registerSignal(int sigNum, QList<int> saMaskList=QList<int>(),bool isSigIGN=false);
    //打开并初始化使用fasync异步通知的设备(配合SIGIO信号)
    int openFasyncIODevice(QString filename,bool readWrite=false);

private:
    //私有构造函数
    SignalHandler(QObject *parent = nullptr);//构造函数私有化
    SignalHandler(const SignalHandler &);//拷贝构造私有化,不定义
    SignalHandler & operator=(const SignalHandler &);//拷贝赋值运算符私有化，不定义

    //信号处理函数必须是静态成员函数，否则信号安装时会因为隐含的this指针造成参数不匹配，无法编译通过。
    static void signalHandle(int sig,siginfo_t *s_t,void *p);

    //采用自管道方案，规避异步信号处理函数的安全问题
    static int pipefd[2];//自管道，[0]读端  [1]写端
    QSocketNotifier *socketNotifier = nullptr;//用来监测自管道读端状态

signals:
    //向外发射收到的信号，sigNum为信号  sigVal对应携带的参数(SIGIO是对应设备的fd)
    void signalReceivedSig(int sigNum,int sigVal);

public slots:
    void pipefdReadyReadSlot(int fd);//处理管道读端数据
};

#endif // SIGNALHANDLER_H
