#pragma once
#include"Epoll.h"
#include<functional>
#include<memory>
#include<unistd.h>
#include<sys/syscall.h>
#include<mutex>
#include<queue>
#include<sys/eventfd.h>
#include"Timestamp.h"
#include<map>
#include"Connection.h"
#include<mutex>
#include<atomic>
class Epoll;
class Connection;
using spConnection=std::shared_ptr<Connection>;

class EventLoop
{
public:
    EventLoop(bool mainloop,int timetvl_=30,int timeout=80);
    ~EventLoop();

    void run();             //运行事件循环
    void stop();
    Epoll *ep();

    void setepolltimeoutcallback(std::function<void(EventLoop*)> fn);
    void removechannel(Channel* ch);
    void updatechannel(Channel* ch);
    bool isinloopthread();

    void queueinloop(std::function<void()> fn);
    void wakeup();
    void handlewakeup();
    void handletimer();             //闹钟响时要做的
    std::mutex& mutex();
    void newconnection(spConnection conn); //在循环中注册这个新的连接

    void settimercallback(std::function<void(int)> fn);
private:
    std::unique_ptr<Epoll> ep_;
    bool mainloop_;
    std::function<void(EventLoop*)> epolltimeoutcallback_;
    size_t threadid_;
    std::queue<std::function<void()>> taskqueue_;
    std::mutex mutex_;
    int wakeupfd_;
    std::unique_ptr<Channel> wakeupchannel_;
    int timerfd_;
    std::unique_ptr<Channel> timerchannel_;
    std::map<int,spConnection> conmp_;
    std::function<void(int)> timercallback_;
    std::mutex mmutex_;  
    int timetvl_;
    int timeout_;       //
    std::atomic<bool> stop_;
};