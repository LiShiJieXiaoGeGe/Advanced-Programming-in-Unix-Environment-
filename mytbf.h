#ifndef _MYTBF_H
#define _MYTBF_H

// 最多能支持1024个不同速率的令牌桶
#define MYTBF_MAX 1024

typedef void mytbf_t;

// 初始化(申请空间)
mytbf_t *init(int cps, int burst);

// 取令牌
int fetch_token(mytbf_t *, int);

// 归还令牌
int return_token(mytbf_t *, int);

// 销毁令牌桶
void destroy(mytbf_t *);

void output(mytbf_t *p);
#endif