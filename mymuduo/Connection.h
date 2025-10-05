#pragma once
#include <functional>
#include "Socket.h"
#include "InetAddress.h"
#include "Channel.h"
#include "EventLoop.h"
#include"Buffer.h"
#include<memory>
#include<sys/syscall.h>
#include<atomic>
#include"Timestamp.h"
#include"http/httprequest.h"
#include"http/httpresponse.h"
#include<sys/uio.h> 
class EventLoop;
class Channel;
class Connection;
using spConnection=std::shared_ptr<Connection>;

class Connection: public std::enable_shared_from_this<Connection>
{
private:

    EventLoop* loop_;       // Connection对应的事件循环，在构造函数中传入。
    std::unique_ptr<Socket> clientsock_;    // 与客户端通讯的Socket。
    std::unique_ptr<Channel> clientchannel_;// Connection对应的channel，在构造函数中创建。
    Buffer inputbuffer_;
    Buffer outputbuffer_;   //接收和发送缓冲区
    std::atomic_bool  disconnect_;  //是否仍然连接
    int iovCnt_;
    struct iovec iov_[2];
    string token_;
    
    std::function<void(spConnection)> closecallback_;
    std::function<void(spConnection)> errorcallback_;
    std::function<void(spConnection)> onmessagecallback_;
    std::function<void(spConnection)> sendcompletecallback_;   //发送数据时的回调函数
    Timestamp lasttime_;
    httprequest request_;
    httprequest::code_status code_;
    httpresponse response_;
public:
    vector<string> messages;
    Connection(EventLoop*ep, std::unique_ptr<Socket>clientsock);
    ~Connection();

    httprequest& getrequest(){return request_;}
    Buffer& getinputbuffer(){return inputbuffer_;}
    string ip()const {return clientsock_->ip();}
	uint16_t port()const{return clientsock_->port();}
	int fd()const{return clientsock_->fd();}
    void onmessage();
    void closecallback();           //这是客户端在主动断开连接的时候channel类回调的函数
    void errorcallback();           //这是客户端在连接错误的时候channel类回调的函数
    void writecallback();
    ssize_t write(int* saveError);
    void setclosecallback(std::function<void(spConnection)> fn);           //这是客户端在主动断开连接的时候channel类回调的函数
    void seterrorcallback(std::function<void(spConnection)> fn);           //这是客户端在连接错误的时候channel类回调的函数
    void setonmessagecallback(std::function<void(spConnection)> fn); 
    void setsendcompletecallback(std::function<void(spConnection)> fn); 

    // 发送数据，不管在任何线程中，都是调用此函数发送数据。
    void send(string& data);
    // 发送数据，如果当前线程是IO线程，直接调用此函数，如果是工作线程，将把此函数传给IO线程。
    void sendInloop(const std::shared_ptr<std::string>&data);
    void send(string&& data) ;
    void sendToLoop(std::shared_ptr<string> message);
    bool timeout(time_t now,int time);

    int do_http_parse();
    void init_400_response(const string& srcDir,  string& path, bool isKeepAlive);
    void init_200_response(const string& srcDir,  string& path, bool isKeepAlive);
    string get_path(){return request_.getpath();}
    bool get_alive(){return request_.get_alive();}
    void make_response();
    void sendcomplete();
};