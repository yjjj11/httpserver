// 网络通讯的客户端程序。
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
using namespace std;
int main(int argc, char *argv[])
{
    // 检查命令行参数数量，需传入 ip 和 port
    if (argc != 3) 
    {
        printf("usage:/client ip port\n");
        printf("example:./client 127.0.0.1 8080\n\n");
        return -1;
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char buf[1024];

    // 创建 TCP socket，失败则提示并退出
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        printf("socket() failed.\n");
        return -1;
    }

    // 初始化服务端地址结构体
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    // 端口转换为网络字节序，从命令行参数获取
    servaddr.sin_port = htons(atoi(argv[2])); 
    servaddr.sin_addr.s_addr=inet_addr(argv[1]);

    // 连接服务端
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) 
    {
        printf("connect() failed: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }
    // cout<<"connect ok\n";
    cout<<"开始时间= "<<time(0)<<"\n";
    char buffer[1024];
     for (int ii = 0; ii < 5; ii++) {
        // 清空缓冲区
        memset(buf, 0, sizeof(buf));

        sprintf(buf,"这是第%d个zyt.",ii);
        // cout<<buf<<"\n";
        char tmpbuf[1024];
        memset(tmpbuf,0,sizeof tmpbuf);
        int len=strlen(buf);
        memcpy(tmpbuf,&len,4);
        memcpy(tmpbuf+4,buf,len);
        // 发送数据到服务端
        int ret=send(sockfd, tmpbuf, len+4, 0);
        if(ret==-1)cout<<"error\n";

        recv(sockfd, &len, 4, 0);   //获取长度

        memset(buf, 0, sizeof(buf));
        ret=recv(sockfd,buf,len,0);
        // 打印服务端响应
        printf("recv:%s\n", buf);
        if(ret==-1)cout<<"error\n";
    }
    // 关闭 socket
    close(sockfd);
    cout<<"结束时间= "<<time(0)<<"\n";
    return 0;
}