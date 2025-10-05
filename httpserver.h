#pragma once
#include "TcpServer.h"
#include "EventLoop.h"
#include "Connection.h"
#include"ThreadPoll.h"
#include<fstream>
#include<sys/stat.h>
#include<sstream>
#include <sys/mman.h>
#include <fcntl.h>
#include<unistd.h>
#include"ConnectionPool.h"
#include"mymuduo/API/api.h"
/*
 EchoServer类：回显服务器
*/
class HttpServer
{
private:
    TcpServer tcpserver_;
    ThreadPool threadpool_;
    ConnectionPool* connpool_;
    string srcDir_;
    API* globl_api_;
public:
    HttpServer(const std::string &ip, const uint16_t port, \
        int subthreadnum=3,int workthreadnum=5);
    ~HttpServer();

    void Start();       // 启动服务。
    void Stop();  

    void HandleNewConnection(spConnection conn);  // 处理新客户端连接请求，在TcpServer类中回调此函数。
    void HandleClose(spConnection conn);  // 关闭客户端的连接，在TcpServer类中回调此函数。
    void HandleError(spConnection conn);  // 客户端的连接错误，在TcpServer类中回调此函数。
    void HandleMessage(spConnection conn);  // 处理客户端的请求报文，在TcpServer类中回调此函数。
    void HandleSendComplete(spConnection conn);  // 数据发送完成后，在TcpServer类中回调此函数。
    //void HandleTimeOut(EventLoop *loop);  // epoll_wait()超时，在TcpServer类中回调此函数。

    void OnMessage(spConnection conn); 

    void do_http_response(spConnection conn,string& path);    
    bool get_http_head(string& realpath,spConnection conn, vector<string>& message);
    
    void not_found(spConnection conn);
    void inner_error(spConnection conn);
    void unimplemented(spConnection conn);
    void bad_request(spConnection conn);


    void send_header(spConnection conn,size_t size);
    void send_content(spConnection conn,size_t size,int& ifs);
};

