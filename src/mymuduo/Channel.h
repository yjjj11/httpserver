#pragma once
#include<sys/epoll.h>
#include"InetAddress.h"
#include"server_socket.h"
#include<functional>
#include"Epoll.h"
#include<iostream>
using namespace std;
using namespace yazi::socket;
class Epoll;
class Channel
{
private:
    int fd_ = -1;  // Channel拥有的fd，Channel和fd是一对一的关系。
    Epoll* ep_ = nullptr;  // Channel对应的红黑树，Channel与Epoll是多对一的关系，一个Channel只对应一个Epoll。
    bool inepoll_ = false;  // Channel是否已添加到epoll树上，如果未添加，调用epoll_ctl()的时候用EPOLL_CTL_ADD，否则用EPOLL_CTL_MOD/EPOLL_CTL_DEL。
    uint32_t events_ = 0;  // fd_需要监视的事件。listenfd和clientfd需要监视EPOLLIN，clientfd还可能需要监视EPOLLOUT。
    uint32_t revents_ = 0;  // fd_已发生的事件。
    bool islistenfd=false;
    function<void()>readcallback_;
    function<void()>closecallback_;
    function<void()>errorcallback_;
    function<void()>writecallback_;
public:
    // 构造函数
    Channel(Epoll* ep, int fd);
    // 析构函数
    ~Channel();

    // 返回fd_成员
    int fd();
    // 采用边缘触发
    void useet();
    // 让epoll_wait()监视fd_的读事件
    void enablereading();
    void disablereading();
    void enablewriting();
    void disablewriting();
    // 把inepoll_成员的值设置为true
    void setinepoll();
    // 设置revents_成员的值为参数ev
    void setrevents(uint32_t ev);
    // 返回inepoll_成员
    bool inpoll();
    // 返回events_成员
    uint32_t events();
    // 返回revents_成员
    uint32_t revents();
    //用于处理epoll_wait()返回时各种情况的判断
    void handleevent();       
    //处理对端发送过来的报文
        
    void disableall();          //取消全部的事件
    void remove();          //从事件循环中删除channel
    //设置fd读事件的回调函数
    void setreadcallback(function<void()> fn);     

    void setclosecallback(function<void()>fn);

    void seterrorcallback(function<void()>fn);

    void setwritecallback(function<void()>fn);


};