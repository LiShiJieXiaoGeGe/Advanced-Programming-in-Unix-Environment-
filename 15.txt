#include "mytbf.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <algorithm>
#include <signal.h>
#include <sys/time.h>
using namespace std;

struct mytbf_st
{
    int cps;
    int burst;
    int token;
    int pos;
};

mytbf_st *job[MYTBF_MAX] = {NULL};

int load_flag = 0;

// 保存初始的信号响应行为
sighandler_t old_signal;

int get_empty_pose()
{
    for (int i = 0; i < MYTBF_MAX; ++i)
    {
        if (job[i] == NULL)
            return i;
    }
    return -1;
}

// 每次来时钟信号，增加每个非空令牌桶的token
void alarm_handler(int s)
{
    for (int i = 0; i < MYTBF_MAX; ++i)
    {
        if (job[i])
        {
            job[i]->token += job[i]->cps;
            if (job[i]->token > job[i]->burst)
                job[i]->token = job[i]->burst;
        }
    }
    // alarm(1);
}

void unload_module()
{
    signal(SIGALRM, old_signal);
    // alarm(0);
    for (int i = 0; i < MYTBF_MAX; ++i)
        free(job[i]);
}

void load_module()
{
    // 注册新的信号响应行为，并将原先信号处理行为保存
    old_signal = signal(SIGALRM, alarm_handler);
    // alarm(1);
    itimerval itv;
    itv.it_interval.tv_sec = 1;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 1;
    itv.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);

    // 钩子函数，模块卸载的时候调用
    atexit(unload_module);
}

// 初始化
mytbf_t *init(int cps, int burst)
{
    printf("in init.................................................\n");
    // 0.第一次调用的时候，注册信号服务（加载模块）
    if (load_flag == 0)
    {
        load_module();
        load_flag = 1;
    }

    // 1.申请空间
    mytbf_st *me = (mytbf_st *)malloc(sizeof(mytbf_st));
    if (me == NULL)
        return NULL;
    me->burst = burst;
    me->cps = cps;
    me->token = 0;

    int pos = -1;
    if ((pos = get_empty_pose()) != -1)
    {
        me->pos = pos;
        job[pos] = me;
    }
    else
    {
        printf("申请失败：令牌桶容量已满\n");
        return NULL;
    }
    printf("out init.................................................\n");
    return me;
}

// 取令牌
int fetch_token(mytbf_t *p, int size)
{
    mytbf_st *sp = (mytbf_st *)p;
    if (size <= 0)
        return -1;
    while (sp->token <= 0)
        pause();
    int n = min(sp->token, size);
    sp->token -= n;
    return n;
}

// 归还令牌
int return_token(mytbf_t *p, int size)
{
    if (size <= 0)
        return -1;
    mytbf_st *sp = (mytbf_st *)p;
    sp->token += size;
    if (sp->token > sp->burst)
        sp->token = sp->burst;
    return size;
}

// 销毁令牌桶
void destroy(mytbf_t *p)
{
    mytbf_st *t = (mytbf_st *)p;
    job[t->pos] = NULL;
    free(p);
}

void output(mytbf_t *p)
{
    mytbf_st *t = (mytbf_st *)p;

    printf("当前令牌桶 cps:%d...........................  \n", t->cps);
    printf("当前令牌桶 burst:%d...........................\n", t->burst);
    printf("当前令牌桶 token:%d...........................\n", t->token);
    printf("当前令牌桶 pos:%d.........................  ..\n", t->pos);
}