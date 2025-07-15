# SignalHandler
这是一个基于Qt编写的便捷类，主要用来响应处理Linux的signal信号。  
## 功能概述
该类主要用来响应处理Linux的signal信号(与Qt的信号槽没有关系)，信号可以来源于内核态的驱动程序(一般使用固定的信号值SIGIO,即fasync异步通知)或者由其他进程(IPC，可以自定义SIGRTMIN--SIGRTMAX信号的功能)发出。  
信号安装使用sigaction()函数，支持信号带参数。信号的处理函数执行类似于软中断，在异步通知时为了确保处理函数的安全性，内部采用自管道方案，通过管道传递信号通知，具体的处理由Qt的QSocketNotifier借助事件循环执行,最终通过Qt的信号槽机制发射信号。
## 代码接口
SignalHandler类设计为单例模式，负责维护全局的signal信号处理，目前仅提供了两个对外接口，分别用于信号注册和fasync设备文件初始化。具体使用示例详见widget类实现。
```
//信号安装注册
void registerSignal(int sigNum, QList<int> saMaskList=QList<int>(),bool isSigIGN=false);
//打开并初始化使用fasync异步通知的设备(配合SIGIO信号)
int openFasyncIODevice(QString filename,bool readWrite=false);
```
为了方便测试，该例程下提供了一个sigqueue目录，该目录下的sigqueue.c文件经过make编译后会生成一个sigqueue可执行命令(用来给指定进程发送指定信号，并可以携带一个额外参数)，然后执行sigqueue.sh脚本（带一个参数0或者1），就可以发送信号(默认是38)了。  


## 作者联系方式
**邮箱:justdoit_mqr@163.com** 
