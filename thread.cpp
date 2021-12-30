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

ps axf 查看进程
ps axm 查看线程
ps ax -L 以linux方式查看线程

并发之线程：
    1.线程
        线程事实上就是用进程号来描述的轻量级进程
        线程标准：posix标准
                openmp标准

        线程标识：pthread_t (posix标准)

        在linux中：是以线程来消耗进程号的，进程实质上就是容器

    2.线程的创建，终止，取消选项，收尸，栈清理
    3.线程同步
    4.线程属性，线程同步的属性
    5.重入、线程和信号、线程与fork


*/

int main(int argc, char **argv)
{
    exit(0);
}