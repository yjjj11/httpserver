#include "Channel.h"
#include"server_socket.h"
#include<functional>
#include"Connection.h"
using namespace std;
// 构造函数实现
Channel::Channel(Epoll* ep, int fd) 
    : ep_(ep), fd_(fd){
    // 构造逻辑可按需扩展，目前仅初始化成员
}

// 析构函数实现
Channel::~Channel() {
    // 不销毁 ep_ 和关闭 fd_，由外部管理生命周期
}

// 获取文件描述符
int Channel::fd() {
    return fd_;
}

// 启用边缘触发（设置 EPOLLET 标志）
void Channel::useet() {
    events_ |= EPOLLET;
}

// 启用读事件监听（添加 EPOLLIN 标志）
void Channel::enablereading() {
    events_ |= EPOLLIN;
    ep_->updatechannel(this);
}

// 标记 Channel 已加入 epoll 树
void Channel::setinepoll() {
    inepoll_ = true;
}

// 设置实际触发的事件（由 Epoll 调用填充）
void Channel::setrevents(uint32_t ev) {
    revents_ = ev;
}

// 检查是否已加入 epoll 树
bool Channel::inpoll() {
    return inepoll_;
}

// 获取期望监听的事件
uint32_t Channel::events() {
    return events_;
}

// 获取实际触发的事件
uint32_t Channel::revents() {
    return revents_;
}

void Channel::handleevent()
{
    // 对方关闭连接事件（检测到对端断开）
    if (revents_ & EPOLLRDHUP) 
    {
        cout<<"close\n";
        closecallback_();
        remove();
    } 
    // 可读事件（普通数据或带外数据）
    else if (revents_ & (EPOLLIN | EPOLLPRI)) 
    { 
        //cout<<"have something to read\n";
        if(readcallback_)
            readcallback_();
    }
    // 可写事件（暂未实现具体逻辑）
    else if (revents_ & EPOLLOUT) 
    { 
        // 后续可添加发送数据的逻辑（如处理发送缓冲区满的情况）
        writecallback_();
    } 
    // 其他事件（视为错误）
    else 
    { 
        
        errorcallback_();
        remove();
    }
}

void Channel::setreadcallback(function<void()> fn)
{
    readcallback_=fn;   //将这个channel的事件设置为读报文事件
}

void Channel::setclosecallback(function<void()>fn)
{
    closecallback_=fn;
}

void Channel::seterrorcallback(function<void()>fn)
{
    errorcallback_=fn;
}

void Channel::disablereading()    // 取消读事件。
{
    events_ &= ~EPOLLIN;
    ep_->updatechannel(this);
}

void Channel::enablewriting()     // 注册写事件。
{
    events_ |= EPOLLOUT;
    ep_->updatechannel(this);
}

void Channel::disablewriting()    // 取消写事件。
{
    events_ &= ~EPOLLOUT;
    ep_->updatechannel(this);
}

void Channel::setwritecallback(function<void()>fn)
{
    writecallback_=fn;
}

void Channel::disableall() {  // 取消全部的事件。
    events_ = 0;
    ep_->updatechannel(this);
}

void Channel::remove() {  // 从事件循环中删除Channel。
    disableall();  // 先取消全部的事件。
    ep_->removechannel(this);  // 从红黑树上删除fd。
}