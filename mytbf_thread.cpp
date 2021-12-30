#include "mytbf.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <algorithm>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
using namespace std;

#define lock(p) pthread_mutex_lock(p)
#define unlock(p) pthread_mutex_unlock(p)
#define mutex_destroy(p) pthread_mutex_destroy(p)

typedef pthread_mutex_t mutex_t;
typedef pthread_cond_t cond_t; // 条件变量
int num = 0;                   // 当前令牌桶数量

//// 多线程实现令牌桶
struct mytbf_st
{
    int cps;
    int burst;
    int token;
    int pos;
    mutex_t mut;
    cond_t con;
};
pthread_t tid_alrm;
pthread_mutex_t mj = PTHREAD_MUTEX_INITIALIZER; // job数组互斥访问
mytbf_st *job[MYTBF_MAX] = {NULL};

pthread_once_t load_once = PTHREAD_ONCE_INIT;

// 约定：使用本函数需要加锁
int get_empty_pose_unlocked()
{
    for (int i = 0; i < MYTBF_MAX; ++i)
    {
        if (job[i] == NULL)
            return i;
    }
    return -1;
}

void *thr_alrm(void *p)
{
    while (1)
    {
        for (int i = 0; i < MYTBF_MAX; ++i)
        {
            lock(&mj);
            if (job[i])
            {
                lock(&job[i]->mut);
                job[i]->token += job[i]->cps;
                if (job[i]->token > job[i]->burst)
                    job[i]->token = job[i]->burst;
                unlock(&job[i]->mut);
                pthread_cond_broadcast(&job[i]->con);
            }
            unlock(&mj);
        }
        sleep(1);
        // printf("\nsleep once..........................................................\n");
    }
}

void unload_module()
{
    mutex_destroy(&mj);
    pthread_cancel(tid_alrm);
    pthread_join(tid_alrm, NULL);
    for (int i = 0; i < MYTBF_MAX; ++i)
        free(job[i]);
}

void load_module()
{
    // printf("进入load_module................................................\n");

    int ret = pthread_create(&tid_alrm, NULL, thr_alrm, NULL);
    if (ret)
    {
        fprintf(stderr, "pthread_create:%s\n", strerror(ret));
        exit(1);
    }
    // 钩子函数，模块卸载的时候调用
    atexit(unload_module);
}

// 初始化
mytbf_t *init(int cps, int burst)
{
    // 0.第一次调用的时候，加载模块
    pthread_once(&load_once, load_module);

    // 1.申请空间
    mytbf_st *me = (mytbf_st *)malloc(sizeof(mytbf_st));
    if (me == NULL)
        return NULL;
    me->burst = burst;
    me->cps = cps;
    me->token = 0;
    pthread_mutex_init(&me->mut, NULL);
    pthread_cond_init(&me->con, NULL);

    int pos = -1;
    lock(&mj);
    if ((pos = get_empty_pose_unlocked()) != -1)
    {
        me->pos = pos;
        job[pos] = me;
        unlock(&mj);
    }
    else
    {
        printf("申请失败：令牌桶容量已满\n");
        unlock(&mj);
        return NULL;
    }
    return me;
}

// 取令牌
int fetch_token(mytbf_t *p, int size)
{
    // printf("进入fetch_token................................................\n");

    mytbf_st *sp = (mytbf_st *)p;
    if (size <= 0)
        return -1;
    lock(&sp->mut);
    while (sp->token <= 0)
    {
        // 1.条件变量实现，通知法
        pthread_cond_wait(&sp->con, &sp->mut);
        // 2.互斥锁实现，忙等法
        // unlock(&sp->mut);
        // sched_yield(); // 短暂出让cpu
        // lock(&sp->mut);
    }
    int n = min(sp->token, size);
    sp->token -= n;
    unlock(&sp->mut);
    return n;
}

// 归还令牌
int return_token(mytbf_t *p, int size)
{
    // printf("进入return_token................................................\n");

    if (size <= 0)
        return -1;
    mytbf_st *sp = (mytbf_st *)p;
    lock(&sp->mut);
    sp->token += size;
    if (sp->token > sp->burst)
        sp->token = sp->burst;
    unlock(&sp->mut);
    return size;
}

// 销毁令牌桶  ??
void destroy(mytbf_t *p)
{
    // printf("进入destroy................................................\n");

    mytbf_st *t = (mytbf_st *)p;
    lock(&mj);
    job[t->pos] = NULL;
    unlock(&mj);
    pthread_mutex_destroy(&t->mut);
    pthread_cond_destroy(&t->con);
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