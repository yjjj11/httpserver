#include "httpserver.h"
#include<signal.h>

HttpServer* httpserver;

// 信号2和15的处理函数，功能是停止服务程序。
void Stop(int sig) 
{
    printf("sig=%d\n", sig);
    httpserver->Stop();
    // 调用HttpServer::Stop()停止服务。
    printf("httpserver已停止。\n");
    delete httpserver;
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 3) 
    {
        printf("usage: ./httpserver ip port\n");
        printf("example: ./httpserver 192.168.150.128 5085\n\n");
        return -1;
    }

    signal(SIGTERM, Stop);  // 信号15，系统kill或killall命令默认发送的信号。
    signal(SIGINT, Stop);   // 信号2，按Ctrl + C发送的信号。

    // 启动服务器
    HttpServer server(argv[1], atoi(argv[2]),6,0);
    server.Start();  // 假设启动失败会抛异常

    return 0;
}