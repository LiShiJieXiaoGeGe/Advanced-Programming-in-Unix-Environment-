#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
using namespace std;

#define DELIMITS " \t\n"
#define FILENAME "/home/xiaojie/apue/test.txt"

/*
    练习：
        1.few: fork exec wait联合使用
        2.shell的简单实现myshell
          区分：shell的内部命令和外部命令
               execvp
               strsep
        3.设置权限 setuid
        4.守护进程 daemon

    ps命令：
        ps axj
        ps axf
    exec函数族:用新的进程映像替换当前的进程映像,进程pid不变（外壳没有变）
        execl()
        execlp()
        execle()
        execv()
        execvp()
        exec函数族使用之前和fork一样要使用fflush刷新缓冲区

    用户权限和组权限实现原理
        root权限下放：
            u+s:设置setuid位
                如果一个程序被设置了setuid位,无论被那个用户启用，都会具有程序所有者的权限
            g+s
        getuid()
        geteuid()

        setuid()
        seteuid()

    观摩：解释器文件（脚本）
        #! /bin/bash
        #! 后面只需要一个可执行程序，然后文件内容将由这个可执行程序进行解释
        ...
    
    system: few的简单封装
            int system(const char *command) 相当于:
            execl("/bin/sh", "sh", "-c", command, (char *) 0)

    进程会计：
        acct() BSD的方言 不可移植
    
    进程时间：
        times()
        clock_t times(struct tms *buf);

    守护进程:(精灵进程)
    {
        ppid为1
        pid sid pgid相等
        脱离终端
    }
        1.脱离控制终端
        2.会话的leader，进程组的leader
        会话 session 标识：sid

        成为守护进程（创建会话）：setsid(子进程来调用)
            getpgrp()
            getpgid()
            setpgid()
    
    系统日志：
        syslogd服务
        openlog()
        syslog()
        closelog()
*/
struct cmd_t
{

    glob_t globres;
};

void few();
void prompt();
void parse(char *line, cmd_t *cmdres);
void myshell();
void setuid_test(int argc, char **argv);
void daemon();
int main(int argc, char **argv)
{
    // few();
    // myshell();
    // setuid_test(argc, argv);
    daemon();
}

void few()
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork error");
        exit(1);
    }

    // 子进程中
    else if (pid == 0)
    {
        printf("in child process\n");

        // 执行可执行文件 从程序到进程：替换当前进程映像，保留pid
        fflush(NULL); // 刷新缓冲区

        // 事实上 第二个参数就相当于给个文件名，是什么并不重要
        int ret = execl("/bin/date", "sleep", NULL);
        perror("execl failed");
        exit(1);
    }

    // 父进程中
    else
    {
        printf("in parent process\n");
    }
    int p;
    wait(&p);
}

//#########################实现shell的简易程序###############################
void myshell()
{
    while (1)
    {
        // 1.打印提示符
        prompt();

        // 2.获取命令参数
        char *linebuf = NULL; //一定记得初始化
        size_t n = 0;

        int ret = getline(&linebuf, &n, stdin);
        if (ret < 0)
        {
            perror("getline() failed");
            exit(1);
        }

        // 3.解析 参数存到 glob_t 类型的变量中
        cmd_t cmd;
        parse(linebuf, &cmd);

        // for (size_t i = 0; i < cmd.globres.gl_pathc; ++i)
        // {
        //     puts(cmd.globres.gl_pathv[i]);
        // }
        //3.判断命令类型
        // 如果是内部命令
        if (0)
        {
        }
        // 如果是外部命令，创建子进程去执行
        else
        {

            pid_t pid = fork();
            if (pid < 0)
            {
                perror("fork failed");
                exit(1);
            }
            // 子进程中，执行exec
            else if (pid == 0)
            {
                execvp(cmd.globres.gl_pathv[0], cmd.globres.gl_pathv);
                perror("exec failed");
                exit(1);
            }
            // 父进程，等着给子进程收尸
            else
            {
                int p;
                wait(&p);
                // sleep(1);
            }
        }
    }
}
//#########################实现shell的简易程序###############################

void prompt()
{
    printf("lsj-shell-$ ");
}

void parse(char *line, cmd_t *cmdres)
{
    // 字符串分割：strsep
    // char *strsep(char **stringp, const char *delim);

    int append = 0;
    while (1)
    {
        char *token = strsep(&line, DELIMITS);
        if (!token)
            break;
        if (token[0] == '\0')
            continue; // 遇到多个分割符

        // 第一次覆盖globres，以后追加（append）
        glob(token, GLOB_NOCHECK | GLOB_APPEND * append, NULL, &(cmdres->globres));
        append = 1;
    }
}

void setuid_test(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage...\n");
        exit(1);
    }
    printf("set之前:父进程uid:%u  euid:%u\n", getuid(), geteuid());
    int ret = setuid(atoi(argv[1]));
    if (ret < 0)
    {
        perror("setuid() failed");
        exit(1);
    }
    printf("set之后:父进程uid:%u  euid:%u\n", getuid(), geteuid());

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork failed");
        exit(1);
    }
    // 子进程
    else if (pid == 0)
    {
        printf("子进程uid:%u  euid:%u\n", getuid(), geteuid());
        execvp(argv[2], argv + 2);
        perror("execvp failed");
        exit(1);
    }
    // 父进程
    else
    {
        wait(NULL);
    }
}

void daemon()
{
    // 产生子进程并关闭父进程
    // 子进程成为守护进程每秒向重定向后的文件打印

    // 连接系统日志server
    openlog("log_test", LOG_PID, LOG_DAEMON);

    fflush(NULL);
    pid_t pid = fork();
    if (pid < 0)
    {
        syslog(LOG_ERR, "fork failed");
        exit(1);
    }
    //
    else if (pid > 0)
    {
        syslog(LOG_INFO, "parent process: will exit");
        exit(0);
    }
    //
    else
    {
        syslog(LOG_INFO, "in child process:");
        sleep(1);
        // 成为守护进程之前，重定向0,1,2文件描述符
        int fd = open("/dev/null", O_RDWR | O_CREAT);
        if (fd < 0)
        {
            syslog(LOG_ERR, "open failed");
            exit(1);
        }
        for (int i = 0; i < 3; ++i)
            dup2(fd, i);
        if (fd > 2)
            close(fd);

        pid_t sid = setsid();

        // 切换进程工作目录
        chdir("/");
        FILE *fp = fopen(FILENAME, "w+");
        for (int i = 0;; ++i)
        {
            fprintf(fp, "%d\n", i);
            fflush(fp);
            sleep(1);
        }
        fclose(fp);
        closelog();
    }
}
