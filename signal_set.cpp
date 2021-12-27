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

/*
    信号集:
        类型：sigset_t

        sigemptyset();
        sigfullset();
        sigaddset();
        sigdelset();
        sigismember();

    信号屏蔽字 /pending集的处理
        int sigprocmask(int how, const sigset_t *set, sigset_t *oldset）
    从信号处理函数中不能轻易往外跳

    扩展：
        信号驱动程序（pause无法完成原子操作）:
        int sigsuspend(const sigset_t *mask);
        用mask位图替换当前进程的位图并阻塞等待信号的到来

    sigaction：用来替换signal函数
        signal函数的缺陷 ！！！：
            1.当多个信号共用同一个信号处理函数时候，由于在响应当前信号时不阻塞其他的信号，容易出事
            2.无法区分信号的来源,如果想仅仅响应内核态发来的信号，将无法区分

    统一用封装后的 sigaction函数:
        int sigaction(int signum, const struct sigaction *act,
                     struct sigaction *oldact);

    struct sigaction {
            void     (*sa_handler)(int);
            // 指定 siginfo_t 中的 si_code 将指定信号来源
            void     (*sa_sigaction)(int, siginfo_t *, void *);
            sigset_t   sa_mask;
            int        sa_flags;
            void     (*sa_restorer)(void);
           };


    实时信号：
        实时信号不会被丢失，新来的实时信号会排队（ulimit -a）查看排队数量

*/

// 1. 信号屏蔽
void sigmask_handler(int num)
{
    write(1, "?", 1);
    siginfo_t si;
}
void test_sigmask()
{
    signal(SIGINT, sigmask_handler);
    sigset_t sset, oset, saveset;
    sigemptyset(&sset);       // 清空信号集
    sigaddset(&sset, SIGINT); // 添加信号

    // 保存之前的设置
    sigprocmask(SIG_UNBLOCK, &sset, &saveset);
    for (int i = 0; i < 1000; ++i)
    {
        // 屏蔽 SIGINT信号: 就是将mask位图中的该信号对应的为置零
        // sigprocmask(SIG_BLOCK, &sset, NULL);
        sigprocmask(SIG_BLOCK, &sset, &oset);
        for (int j = 0; j < 5; ++j)
        {
            write(1, "#", 1);
            sleep(1);
        }
        write(1, "\n", 1);
        // 解除屏蔽 SIGINT信号：将mask位图中该信号对应的位置一
        // sigprocmask(SIG_UNBLOCK, &sset, NULL);
        sigprocmask(SIG_SETMASK, &oset, NULL);
    }
    // 恢复之前的设置
    sigprocmask(SIG_SETMASK, &saveset, NULL);
}
void test_sigsuspend()
{
    signal(SIGINT, sigmask_handler);
    sigset_t sset, oset, saveset;

    sigemptyset(&sset);       // 清空信号集
    sigaddset(&sset, SIGINT); // 添加信号

    // saveset 保存之前的设置
    sigprocmask(SIG_UNBLOCK, &sset, &saveset);
    sigprocmask(SIG_BLOCK, &sset, &oset); // oset保存对SIGINT unblocked之后的信号位图

    for (int i = 0; i < 1000; ++i)
    {
        // sigprocmask(SIG_BLOCK, &sset, &oset);
        for (int j = 0; j < 5; ++j)
        {
            write(1, "#", 1);
            sleep(1);
        }
        write(1, "\n", 1);
        // 用一下sleep说明问题：sigprocmask与pause不是原子操作
        // 当将信号解除阻塞后，还未执行到pause就已经被响应，程序将卡在pause处
        // sigprocmask(SIG_SETMASK, &oset, NULL);
        // sleep(10);
        // pause();

        // sigsunspend:用oset位图替换当前进程的位图并阻塞等待信号的到来
        sigsuspend(&oset);
    }
    // 恢复之前的设置
    sigprocmask(SIG_SETMASK, &saveset, NULL);
}

// 2.signal存在的问题:多个信号共用同一个处理函数
FILE *fp = NULL;
void signal_problem_handler(int s)
{
    printf("当前信号值%d\n", s);

    // ！！！ 多次释放内存将报错
    if (fp)
    {
        printf("信号%d准备释放内存\n", s);
        sleep(10);
        fclose(fp);
        fp = NULL;
    }
}
void signal_problem()
{
    // signal存在的问题：
    signal(SIGINT, signal_problem_handler);
    signal(SIGQUIT, signal_problem_handler);

    // 解决方案：用sigaction代替实现
    struct sigaction sa;
    // sigemptyset(&sa.sa_mask);
    // sigaddset(&sa.sa_mask, SIGINT);
    // sigaddset(&sa.sa_mask, SIGQUIT);
    // sa.sa_flags = 0;
    // sa.sa_handler = signal_problem_handler;
    // sigaction(SIGINT, &sa, NULL);
    // sigaction(SIGQUIT, &sa, NULL);

    fp = fopen("./test.txt", "r");
    if (!fp)
    {
        printf("open failed\n");
        return;
    }
    sleep(1000);
}

int main(int argc, char **argv)
{
    // test_sigmask();
    // test_sigsuspend();
    // signal_problem();

    return 0;
}