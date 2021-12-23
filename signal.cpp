#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
using namespace std;

#define CIR 10

// 令牌桶的令牌上限
#define BURST 300

#define BUF_SIZE 1024
/*
    练习：
        1.slowcat(实现流量控制功能)

    一、信号
        1.信号的概念：软件中断，信号的响应依赖于中断机制
            并发：异步
            异步事件的处理：轮询法 通知法（中断）

        2.signal()
            typedef void (*sighandler_t)(int);
            sighandler_t signal(int signum, sighandler_t handler);

            信号会打断阻塞的系统调用！！！(比如打断sleep,open等)

        3.信号的不可靠
            信号行为不可靠：信号回调函数由内核布置，接收到连续的信号时，有可能被内核多次调用，冲掉前一次的运行环境

        4.可重入函数：可以被中断的函数
            所有的系统调用都是可重入的
            部分库函数可重入 _r版本

        5.信号的响应过程
            标准信号从收到(改变pending位图)到响应(在内核态转用户态的路上进行pending&mask)有一个不可避免的延迟
            思考：如何忽略掉一个信号的？(mask位图该位置零)
                 标准信号为什么要丢失？
                 标准信号的响应没有顺序
        6.常用信号

            kill:向一个进程（组）发信号
                int kill(pid_t pid, int sig);
                pid > 0 ：给pid发信号
                pid = 0 : 进程组内广播
                pid = -1: 全局广播
            raise():给当前进程发信号 相当于 kill(getpid(),sig)

            alarm(uint seconds):
                闹钟:seconds秒后发送SIGALRM信号(默认杀掉进程)
                赋予程序时间观念

            pause():使当前进程进入阻塞态，放弃处理器，等待信号来打断

        7. volatile关键字的使用
        8. alarm案例：限流算法的实现，令牌桶与漏桶

        9. 建议使用setitimer代替alarm，可以避免使用alarm链
            int setitimer(int which, const struct itimerval *new_value,
                     struct itimerval *old_value);

        10. abrt():给当前进程发一个SIGABRT信号，认为制造异常，结束当前进程
        11. system的使用：阻塞SIGCHLD信号，忽略SIGINT，SIGQUIT信号
        12. sleep：尽量不用，因为有些平台是用 alarm+pause 封装的
            usleep
            select的副作用

*/
void handler(int signum);
void handler2(int signum);
void slowcat_handler(int signum);
void token_handler(int signum);
void signal_test();
void alarm_test();
void kill_test();
void slowcat(int argc, char **argv);        // 漏桶：alarm
void slowcat_token(int argc, char **argv);  // 限流算法实现
void slowcat_itimer(int argc, char **argv); // 漏桶：setitimer

int loop = 0; // 限流算法标志位
int token = 0;
int main(int argc, char **argv)
{
    // printf("in main... 当前进程pid:%d\n", getpid());

    // signal_test();
    // alarm_test();
    // kill_test();
    // slowcat(argc, argv);
    // slowcat_token(argc, argv);
    slowcat_itimer(argc, argv);
    return 0;
}
void kill_test()
{
    printf("in kill_test\n");

    kill(getpid(), SIGALRM);
    while (1)
    {
    }
    printf("out kill_test\n");

    exit(0);
}
void slowcat_handler(int signum)
{
    loop = 1;
    alarm(1);
}

// 限流算法：漏桶的实现
void slowcat(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "argc < 2\n");
        exit(1);
    }
    char *src = argv[1];

    int fd;
    // 循环打开文件，避免文件打开过程中被信号打断
    do
    {
        fd = open(src, O_RDONLY);
        if (fd < 0 && errno != EINTR)
        {
            perror("open failed");
            exit(1);
        }
    } while (fd < 0);

    // 读文件并写入（读多少，写多少）
    char buf[BUF_SIZE];
    bzero(buf, BUF_SIZE);
    signal(SIGALRM, slowcat_handler); // 注册
    alarm(1);
    int len = 0; // 读取的长度
    int fread = 0;
    while (1)
    {
        while (loop == 0)
        {
            pause(); // 等待信号到来
        }
        loop = 0;
        do
        {
            if (fread % 5 == 0)
            {
                int s = 10;
                // 延时s秒，套在循环中避免对信号打断
                while ((s = sleep(s)) != 0)
                    ;
            }
            fread++;
            len = read(fd, buf, CIR);
            if (len < 0)
            {
                if (errno == EINTR)
                    continue;
                else
                {
                    perror("read wrong");
                    break;
                }
            }
        } while (len < 0);
        if (len <= 0)
            break;
        else
        {
            int pos = 0;
            while (len > 0)
            {
                int ret = write(STDOUT_FILENO, buf + pos, len);
                if (ret < 0)
                {
                    if (errno == EINTR)
                        continue;
                    else
                    {
                        perror("write false");
                        break;
                    }
                }
                else if (ret == 0)
                    break;
                else
                {
                    pos += ret;
                    len -= ret;
                }
            }
        }
    }
    close(fd);
}

void token_handler(int signum)
{
    token++;
    if (token > BURST)
        token = BURST;
    alarm(1);
}

void slowcat_itimer_handler(int signum)
{
    loop = 1;
}
void slowcat_itimer(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "argc < 2\n");
        exit(1);
    }
    char *src = argv[1];

    int fd;
    // 循环打开文件，避免文件打开过程中被信号打断
    do
    {
        fd = open(src, O_RDONLY);
        if (fd < 0 && errno != EINTR)
        {
            perror("open failed");
            exit(1);
        }
    } while (fd < 0);

    // 读文件并写入（读多少，写多少）
    char buf[BUF_SIZE];
    bzero(buf, BUF_SIZE);
    signal(SIGALRM, slowcat_itimer_handler); // 注册

    // 这里使用setitimer代替 alarm链：每次时间经过itv.it_value就将itv.it_interval重新赋值给itv.it_val
    itimerval itv;
    itv.it_interval.tv_sec = 1;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 1;
    itv.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);

    int len = 0; // 读取的长度
    int fread = 0;
    while (1)
    {
        while (loop == 0)
        {
            pause(); // 等待信号到来
        }
        loop = 0;
        do
        {
            // if (fread % 5 == 0)
            // {
            //     int s = 10;
            //     // 延时s秒，套在循环中避免对信号打断
            //     while ((s = sleep(s)) != 0)
            //         ;
            // }
            // fread++;
            len = read(fd, buf, CIR);
            if (len < 0)
            {
                if (errno == EINTR)
                    continue;
                else
                {
                    perror("read wrong");
                    break;
                }
            }
        } while (len < 0);
        if (len <= 0)
            break;
        else
        {
            int pos = 0;
            while (len > 0)
            {
                int ret = write(STDOUT_FILENO, buf + pos, len);
                if (ret < 0)
                {
                    if (errno == EINTR)
                        continue;
                    else
                    {
                        perror("write false");
                        break;
                    }
                }
                else if (ret == 0)
                    break;
                else
                {
                    pos += ret;
                    len -= ret;
                }
            }
        }
    }
    close(fd);
}

// 限流算法：令牌桶的实现
void slowcat_token(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "argc < 2\n");
        exit(1);
    }
    char *src = argv[1];
    void token_handler(int signum);

    int fd;
    // 循环打开文件，避免文件打开过程中被信号打断
    do
    {
        fd = open(src, O_RDONLY);
        if (fd < 0 && errno != EINTR)
        {
            perror("open failed");
            exit(1);
        }
    } while (fd < 0);

    // 读文件并写入（读多少，写多少）
    char buf[BUF_SIZE];
    bzero(buf, BUF_SIZE);
    signal(SIGALRM, token_handler); // 注册
    alarm(1);
    int len = 0;   // 读取的长度
    int fread = 0; // 第一次读取时，延时，观察令牌桶
    while (1)
    {
        while (token <= 0)
        {
            pause(); // 等待信号到来
        }
        // printf("当前token：%d\n", token);
        token--;

        do
        {
            if (fread % 5 == 0)
            {
                int s = 10;
                // 延时s秒，套在循环中避免对信号打断
                while ((s = sleep(s)) != 0)
                    ;
            }
            fread++;
            len = read(fd, buf, CIR);
            if (len < 0)
            {
                if (errno == EINTR)
                    continue;
                else
                {
                    perror("read wrong");
                    break;
                }
            }
        } while (len < 0);
        if (len <= 0)
        {
            break;
        }
        else
        {
            int pos = 0;
            while (len > 0)
            {
                int ret = write(STDOUT_FILENO, buf + pos, len);
                if (ret < 0)
                {
                    if (errno == EINTR)
                        continue;
                    else
                    {
                        perror("write false");
                        break;
                    }
                }
                else if (ret == 0)
                    break;
                else
                {
                    pos += ret;
                    len -= ret;
                }
            }
        }
    }
    close(fd);
}

void alarm_test()
{
    printf("五秒后我将结束自己\n");
    alarm(5);
    while (1)
    {
    };
    exit(0);
}
void signal_test()
{
    printf("in signal_test... 当前进程pid:%d\n", getpid());

    // signal完成的只是注册功能:规定当信号来临时产生什么动作

    // signal(SIGINT, SIG_DFL);
    // signal(SIGINT, SIG_IGN); // (信号类型，信号处理方式)
    signal(SIGINT, handler);

    for (int i = 0; i < 5; ++i)
    {
        write(STDOUT_FILENO, "#", 1);
        unsigned seconds = 30;
        while ((seconds = sleep(seconds)) != 0)
        {
        }
        // sleep(1); //  信号会打断阻塞的系统调用！！！
    }
}
void handler(int signum)
{
    printf("in handler...\n");
    signal(SIGQUIT, handler2);

    while (1)
    {
    }

    // write(STDOUT_FILENO, "?", 1);
}

void handler2(int signum)
{
    printf("in handler2...\n");
    printf("out handler2...\n");
}