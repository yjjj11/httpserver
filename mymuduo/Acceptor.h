#pragma once
#include <functional>
#include "Socket.h"
#include "InetAddress.h"
#include "Channel.h"
#include "EventLoop.h"
#include<memory>
class Acceptor
{
private:
    EventLoop* loop_;         // Acceptor对应的事件循环，在构造函数中传入。
    ServerSocket *servsock_;       // 服务端用于监听的socket，在构造函数中创建。
    Channel acceptchannel_; // Acceptor对应的channel，在构造函数中创建。
    std::function<void(std::unique_ptr<Socket>)> newconnectioncb_;  //设置回调函数
public:
    Acceptor(EventLoop* loop, const std::string &ip, const uint16_t port);
    ~Acceptor();

    void newconnection();//用于处理新的连接请求
    void setnewconnection(std::function<void(std::unique_ptr<Socket>)> fn);    //设置处理新客户端的回调函数
};