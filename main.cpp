#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "mytbf.h"

#define BUF_SIZE 1024
using namespace std;

// 20个进程竞争令牌桶，数据写入20个文件

char *src = NULL; // 源文件路径
pthread_t ts[20];
void *mytbf_test(void *p)
{
    int fd, fd2;
    int val = *(int *)p;
    printf("当前线程val:%d\n", val);
    // 循环打开文件，避免文件打开过程中被信号打断
    do
    {
        fd = open(src, O_RDONLY);
        if (fd < 0 && errno != EINTR)
        {
            perror("\nopen failed////////////////////////////////////////////\n");
            exit(1);
        }
    } while (fd < 0);
    do
    {
        char dst[100];
        sprintf(dst, "./%d.txt", val);

        // open中有O_CREAT选项需要有第三个参数：权限
        fd2 = open(dst, O_RDWR | O_TRUNC | O_CREAT, 0666);
        if (fd2 < 0 && errno != EINTR)
        {
            // perror("open failed");
            perror("\nopen failed////////////////////////////////////////////\n");

            exit(1);
        }
    } while (fd2 < 0);
    // 读文件并写入（读多少，写多少）
    char buf[BUF_SIZE];
    bzero(buf, BUF_SIZE);

    // 1.申请令牌桶
    mytbf_t *tbf = init(10, 100);
    printf("\n%d线程init令牌桶结束.............................................\n", val);

    if (tbf == NULL)
    {
        fprintf(stderr, "令牌桶申请失败\n");
        exit(1);
    }

    int len = 0; // 读取的长度
    while (1)
    {
        // 2.取令牌
        int n = -1;
        while ((n = fetch_token(tbf, BUF_SIZE)) == -1)
            ; // 直到取到令牌才结束循环
        printf("\n%d当前取到令牌：%d个.............................................\n", val, n);
        do
        {
            len = read(fd, buf, n);
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
                int ret = write(fd2, buf + pos, len);
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
            // 3.写完之后要归还令牌桶
            return_token(tbf, n);
            printf("\n%d当前归还令牌：%d个.............................................\n", val, n);
        }
    }
    pthread_exit(NULL);
    close(fd);
    close(fd2);
    // 4.销毁令牌桶
    destroy(tbf);
    printf("\n%d当前销毁令牌桶.............................................\n", val);
}
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "argc < 2\n");
        exit(1);
    }

    src = (char *)argv[1];
    printf("ok?\n");
    for (int i = 0; i < 20; i++)
    {
        int *pi = new int;
        *pi = i;
        printf("\n创建第%d个线程\n", i + 1);
        pthread_create(ts + i, NULL, mytbf_test, (void *)pi);
    }

    for (int i = 0; i < 20; i++)
    {
        pthread_join(ts[i], NULL);
    }
    return 0;
}