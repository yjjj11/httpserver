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

    try {
        // 转换并检查端口
        int port = std::stoi(argv[2]);
        if (port < 1 || port > 65535) {
            throw "端口必须在 1-65535 之间";
        }

        // 启动服务器
        HttpServer server(argv[1], port,3,0);
        server.Start();  // 假设启动失败会抛异常

    } catch (const char* msg) {
        // 处理自定义错误信息
        std::cerr << "启动失败: " << msg << std::endl;
        return 1;
    } catch (...) {
        // 捕获所有其他异常
        std::cerr << "启动失败: 发生未知错误" << std::endl;
        return 1;
    }

    return 0;
}