#include "EchoServer.h"
#include<signal.h>

EchoServer* echoserver;

// 信号2和15的处理函数，功能是停止服务程序。
void Stop(int sig) 
{
    printf("sig=%d\n", sig);
    echoserver->Stop();
    // 调用EchoServer::Stop()停止服务。
    printf("echoserver已停止。\n");
    delete echoserver;
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 3) 
    {
        printf("usage: ./echoserver ip port\n");
        printf("example: ./echoserver 192.168.150.128 5085\n\n");
        return -1;
    }

    signal(SIGTERM, Stop);  // 信号15，系统kill或killall命令默认发送的信号。
    signal(SIGINT, Stop);   // 信号2，按Ctrl + C发送的信号。

    echoserver = new EchoServer(argv[1], atoi(argv[2]), 3, 0);
    echoserver->Start();

    return 0;
}