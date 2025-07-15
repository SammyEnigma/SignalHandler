/*
 *@author: 缪庆瑞
 *@date:   2019.6.24
 *@update: 2025.7.13
 *@brief:  负责Linux信号的响应处理
 */
#include "signalhandler.h"
#include <stdio.h>
#include <fcntl.h>
#include <QDebug>
#include <QThread>

int SignalHandler::pipefd[2]={-1,-1};
/*
 *@brief:   构造函数(私有)
 *@date:    2025.07.13
 */
SignalHandler::SignalHandler(QObject *parent): QObject(parent)
{
    //创建自管道
    if(pipe(pipefd) != -1)
    {
        //设置读端为非阻塞(避免read阻塞)
        fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
        //使用QSocketNotifier监测读端事件
        socketNotifier = new QSocketNotifier(pipefd[0],QSocketNotifier::Read,this);
        connect(socketNotifier,&QSocketNotifier::activated,this,&SignalHandler::pipefdReadyReadSlot);
    }
    else
    {
        perror("pipe");
    }

    qDebug()<<"main thread:"<<QThread::currentThread();
}
/*
 *@brief:  析构函数(负责释放资源)
 *@date:   2025.07.13
 */
SignalHandler::~SignalHandler()
{
    if(socketNotifier)
    {
        socketNotifier->setEnabled(false);
        ::close(pipefd[0]);
        ::close(pipefd[1]);
    }
}
/*
 *@brief:   信号注册
 *@date:    2025.07.13
 *@param:   sigNum:信号值，针对linux的64个信号(kill -l)，如果是用户自定义信号建议选38之后
 *          SIGKILL 和 SIGSTOP是特殊信号，不可以注册处理或者忽略,另外对同一信号多次注册则覆盖之前的处理
 *@param:   saMaskList:阻塞信号列表，表示在注册信号处理函数执行期间，阻塞对应发生的信号，处理结束后自动解除
 *          注意阻塞信号并非完全屏蔽，仅在handler处理期间阻塞，结束后阻塞的信号还是会触发,另外注册的信号自身
 *          默认会被阻塞(除非设置了 SA_NODEFER 标志），无需显式添加到 sa_mask(阻塞自身信号只是保证当前正在
 *          handler处理的线程不会再次被信号中断形成递归嵌套，但多线程环境还是会将相同信号分发给其他未阻塞该信
 *          号的线程处理)
 *@param:   isSigIGN:是否选择忽略信号,一般默认的SIG_DFL动作是结束进程
 */
void SignalHandler::registerSignal(int sigNum, QList<int> saMaskList, bool isSigIGN)
{
    struct sigaction act;//用来注册信号
    if(isSigIGN)
    {
        act.sa_handler = SIG_IGN;
    }
    else
    {
        act.sa_sigaction = signalHandle;//指定回调函数，三个函参，能接受额外数据
        act.sa_flags = SA_SIGINFO;//与sa_sigaction搭配使用，接受额外数据
        sigemptyset(&act.sa_mask);//清空阻塞信号掩码
        //添加指定的屏蔽信号
        for(int i= 0;i<saMaskList.size();i++)
        {
            sigaddset(&act.sa_mask,saMaskList.at(i));
        }
    }
    //为sigNum指定信号处理函数
    sigaction(sigNum,&act,NULL);
}
/*
 *@brief:  打开并初始化使用fasync异步通知的设备
 *该函数配合SIGIO信号，用来打开设备，绑定信号与进程，并设置对应设备的异步通知特性
 *@date:   2025.07.13
 *@param:  filename:设备文件名，包含路径
 *@param:  readWrite:true=读写 false=只读(默认)
 *@return: int:设备文件句柄,-1表示失败,句柄需要外部进行读写和资源管理
 */
int SignalHandler::openFasyncIODevice(QString filename, bool readWrite)
{
    int oflag = readWrite?O_RDWR:O_RDONLY;
    int fd = ::open(filename.toLatin1().constData(),oflag|O_NONBLOCK);
    if(fd >= 0)
    {
        //设置文件描述符的拥有者，即该设备文件内核驱动的SIGIO信号指定发给本进程
        if(fcntl(fd,F_SETOWN,getpid()) == -1)
        {
            perror("fasync device F_SETOWN failed.");
            ::close(fd);
            return -1;
        }
        //为设备启用异步通知特性
        int flags = fcntl(fd, F_GETFL);
        if(fcntl(fd,F_SETFL,flags|FASYNC) == -1)
        {
            perror("fasync device FASYNC failed.");
            ::close(fd);
            return -1;
        }
        return fd;
    }
    else
    {
        perror("fasync device open failed.");
        return -1;
    }
}
/*
 *@brief:   信号处理函数(软中断处理函数)
 * 1.该函数由信号触发，会中断当前程序(其他函数)的执行，等处理结束再回到中断之前的位置继续运行，所以处理不宜太繁琐，
 * 以保证尽快恢复到被打断之前的状态，避免中断前的执行逻辑受影响。
 * 2.如果触发信号是内部的同步信号(比如SIGSEGV)，则会在触发该信号的线程中执行。如果信号是外部异步信号通知，则内核会
 * 调度一个未阻塞该信号的线程去处理，多线程情况下执行该函数的线程不可预测(实测当某个线程休眠或者阻塞时确实会调度其他
 * 线程执行)。所以一定要避免在信号处理函数中调用非异步信号安全的函数，否则可能导致问题(竞争条件、死锁、内存损坏)。
 * 注:sigaction这种注册信号处理函数的方式，本质上不适合多线程。通常都推荐换用sigwait()同步等待信号的方式(在主线程中
 * 通过pthread_sigmask(SIG_BLOCK, &set, NULL)阻塞信号，保证其他子线程后启动继承阻塞掩码，然后再开一个单独的子线程
 * 通过sigwait()同步等待信号)，这样可以避免异步信号处理的安全问题，但该方式只能简单的传递信号本身，不能携带额外数据。
 * 还有一种signalfd的方案，但又不支持SIGIO(fasync),所以最终还是采用sigaction方案，通过结合自管道规避异步处理函数的
 * 安全问题解决。
 *@date:    2025.07.13
 *@param:   sig:信号值
 *@param:   s_t:信号所携带的额外数据
 *@param:   p:posix没有规范使用该参数的标准,暂不使用
 */
void SignalHandler::signalHandle(int sig, siginfo_t *s_t, void *p)
{
    Q_UNUSED(p);//该参数暂时不用

    //调试handler的线程执行情况，注：printf，qDebug都是非异步安全函数，生产环境不要使用
    //qDebug()<<"signalHandle thread:"<<QThread::currentThread();
    //sleep(2);

    //不同信号携带信息不同，这里默认使用si_int作为默认参数
    int sig_info = s_t->si_int;
    //对于内核驱动常用的SIGIO(fasync)通知，使用si_fd作为附加参数，该值对应打开的驱动设备句柄，用于外部区分是哪个驱动触发了信号
    if (sig == SIGIO && s_t->si_code == POLL_IN)
    {
        sig_info = s_t->si_fd;
    }

    /*该函数内必须使用异步安全函数，保证安全性
     *因为write函数是异步安全函数，以及管道写入的原子性(PIPE_BUF)，所以这里使用自管道方案
     *定义一个buf[8],内部规则定义前四个字节表示信号值，后四个字节表示携带的额外参数*/
    //将sig信号值按照大端序存放到前四个字节
    char write_buf[8]={0};
    write_buf[3] = sig&0xFF;
    //将sig信号携带的额外数据(这里固定用int)按照大端序放到后四个字节
    write_buf[4] = (sig_info >> 24) & 0xFF;
    write_buf[5] = (sig_info >> 16) & 0xFF;
    write_buf[6] = (sig_info >> 8) & 0xFF;
    write_buf[7] = (sig_info) & 0xFF;

    //写管道
    write(pipefd[1],write_buf,sizeof(write_buf));
}
/*
 *@brief:  自管道读端有数据可读
 *@date:   2025.07.13
 *@param:  fd:自管道读端句柄
 */
void SignalHandler::pipefdReadyReadSlot(int fd)
{
    char read_buf[8] = {0};
    int read_size = read(fd,read_buf,sizeof(read_buf));
    while(read_size == 8)
    {
        //根据写管道定义的规则读取数据
        int sigNum = read_buf[3];
        int sigVal = (read_buf[4]<<24) | (read_buf[5]<<16) | (read_buf[6]<<8) | read_buf[7];
        emit signalReceivedSig(sigNum,sigVal);

        read_size = read(fd,read_buf,sizeof(read_buf));
    }
}
