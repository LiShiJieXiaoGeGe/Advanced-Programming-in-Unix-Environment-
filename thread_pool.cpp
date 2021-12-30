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
#include <pthread.h>
#include <math.h>
using namespace std;

#define maxn 100000
#define BUF_SIZE 1024
/*
    通知法：条件变量(避免忙等)
        pthread_cond_t
        pthread_cond_init()
        pthread_cond_destroy()
        pthread_cond_breadcast()
        pthread_cond_signal()
        int pthread_cond_wait(pthread_cond_t *restrict cond,pthread_mutex_t *restrict mutex);
            首先确保mutex上锁，pthread_cond_wait将解锁并在临界区外等待，知道pthread_cond_breadcast()或者
            pthread_cond_signal()到来，pthread_cond_wait将抢锁并进入临界区查看

        pthread_cond_timed_wait()

    信号量：

*/
//// 1.线程任务池的实现： \
    上游main线程每次投放数字num，交给下游的三个线程争抢 \
    num > 0:当前待解决任务；\
    num == 0：任务已被下游线程接受，待上游投放任务; \
    num < 0: 提醒下游进程任务已结束
int num = 0;
pthread_mutex_t mnum, m1, m2; // mnum互斥，m1,m2同步
void *isprime(void *p)
{
    while (1)
    {
        bool prime = true;
        pthread_mutex_lock(&m2);
        pthread_mutex_lock(&mnum);
        int val = num; // 取任务
        if (num == -1)
        {
            printf("检测到num为-1\n");
            pthread_mutex_unlock(&mnum);
            pthread_mutex_unlock(&m1);
            pthread_mutex_unlock(&m2);
            break;
        }
        num = 0;
        pthread_mutex_unlock(&mnum);
        pthread_mutex_unlock(&m1);
        if (val < 2)
            prime = false;
        else
        {
            for (int i = 2; i <= sqrt(val * 1.0); ++i)
            {
                if (val % i == 0)
                {
                    prime = false;
                    break;
                }
            }
        }
        if (prime)
            printf("%d is prime,current tid:%u\n", val, pthread_self());
    }
    pthread_exit(p);
}
void task_pool()
{
    pthread_t ts[3];
    pthread_mutex_init(&mnum, NULL);
    pthread_mutex_init(&m1, NULL);
    pthread_mutex_init(&m2, NULL);
    pthread_mutex_lock(&m2);
    for (int i = 0; i < 3; ++i)
    {
        int *pi = new int;
        *pi = i;
        pthread_create(ts + i, NULL, isprime, (void *)pi);
    }
    // 发放任务，从1到1000
    for (int i = 1; i <= 1000; ++i)
    {
        // 操作num
        pthread_mutex_lock(&m1);
        pthread_mutex_lock(&mnum);
        if (num == 0)
            num = i;
        pthread_mutex_unlock(&mnum);
        pthread_mutex_unlock(&m2);
    }
    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&mnum);
    if (num == 0)
        num = -1;
    pthread_mutex_unlock(&mnum);
    pthread_mutex_unlock(&m2);

    for (int i = 0; i < 3; ++i)
    {
        void *pi = NULL;
        pthread_join(ts[i], &pi);
    }
}
int main(int argc, char **argv)
{
    task_pool();
    exit(0);
}