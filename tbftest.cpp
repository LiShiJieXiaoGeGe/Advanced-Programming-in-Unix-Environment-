#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "mytbf.h"

#define BUF_SIZE 1024
using namespace std;

int main(int argc, char **argv)
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

    // 1.申请令牌桶
    mytbf_t *tbf = init(10, 100);
    if (tbf == NULL)
    {
        fprintf(stderr, "令牌桶申请失败\n");
        return 0;
    }

    int len = 0; // 读取的长度
    while (1)
    {
        // 2.取令牌
        int n = -1;
        while ((n = fetch_token(tbf, BUF_SIZE)) == -1)
            ; // 直到取到令牌才结束循环
        // printf("当前取到令牌：%d个..............................\n", n);
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
            // 3.写完之后要归还令牌桶
            return_token(tbf, n);
        }
        // output(tbf);
    }
    close(fd);
    // 4.销毁令牌桶
    destroy(tbf);

    return 0;
}