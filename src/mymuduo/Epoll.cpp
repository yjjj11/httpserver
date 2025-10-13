#include"Epoll.h"
#include<iostream>
using namespace std;
Epoll::Epoll()  // 在构造函数中创建了epollfd_。
{
    if ((epollfd_ = epoll_create(1)) == -1)  // 创建epoll句柄（红黑树）。
    {
        printf("epoll_create() failed(%d).\n", errno); 
        exit(-1);
    }
}

Epoll::~Epoll()  // 在析构函数中关闭epollfd_。
{
    close(epollfd_);
}

// void Epoll::addfd(int fd, uint32_t op)  // 把fd和它需要监视的事件添加到红黑树上。
// {
//     epoll_event ev;  // 声明事件的数据结构。
//     ev.data.fd = fd;  // 指定事件的自定义数据，会随着epoll_wait()返回的事件一并返回。
//     ev.events = op;  // 让epoll监视fd的。

//     if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev) == -1)  // 把需要监视的fd和它的事件加入epollfd中。
//     {
//         printf("epoll_ctl() failed(%d).\n", errno); 
//         exit(-1);
//     }
// }
std::vector<Channel*> Epoll::loop(int timeout)  // 运行epoll_wait()，等待事件的发生，已发生的事件用vector容器返回。
{
    std::vector<Channel*> Channels;  // 存放epoll_wait()返回的事件。

    bzero(events_, sizeof(events_));
    int infds = epoll_wait(epollfd_, events_, MaxEvents, timeout);  // 等待监视的fd有事件发生。

    // 返回失败。
    if (infds < 0)
    {
        perror("epoll_wait() failed"); 
        exit(-1);
    }
    // 如果infds>0，表示有事件发生的fd的数量。
    for (int ii = 0; ii < infds; ii++)  // 遍历epoll返回的数组evs。
    {
        Channel* ch=(Channel*)events_[ii].data.ptr;
        ch->setrevents(events_[ii].events);
        Channels.push_back(ch);
    }
    return Channels;
}

void Epoll::updatechannel(Channel* ch)
{
    epoll_event ev;
    ev.data.ptr=ch;
    ev.events=ch->events();

    if(ch->inpoll())//已经在树上
    {
        if(epoll_ctl(epollfd_,EPOLL_CTL_MOD,ch->fd(),&ev)==-1)
        {
            cout<<"epoll_ctl() mod failed\n";
            exit(-1);
        }
    }
    else{
        if(epoll_ctl(epollfd_,EPOLL_CTL_ADD,ch->fd(),&ev)==-1)
        {
            cout<<"epoll_ctl() add failed\n";
            exit(-1);
        }
        ch->setinepoll();
    }
}

void Epoll::removechannel(Channel *ch)
{
    if (ch->inpoll())  // 如果channel已经在树上了。
    {
        //printf("removechannel()\n");

        if (epoll_ctl(epollfd_, EPOLL_CTL_DEL, ch->fd(), 0) == -1)
        {
            perror("epoll_ctl() failed.\n"); 
            exit(-1);
        }
    }
}