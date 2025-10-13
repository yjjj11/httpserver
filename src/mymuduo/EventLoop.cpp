#include"EventLoop.h"
#include <sys/syscall.h>  
#include <unistd.h> 

int createtimerfd(int sec = 30) {
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK); 
    struct itimerspec timeout; 
    memset(&timeout, 0, sizeof(struct itimerspec));
    timeout.it_value.tv_sec = sec; 
    timeout.it_value.tv_nsec = 0;
    timerfd_settime(tfd, 0, &timeout, 0);
    return tfd;
}
EventLoop::EventLoop(bool mainloop,int timetvl,int timeout):
ep_(new Epoll),mainloop_(mainloop),timetvl_(timetvl),timeout_(timeout),\
wakeupfd_(eventfd(0,EFD_NONBLOCK)),stop_(false),\
wakeupchannel_(new Channel(ep_.get(),wakeupfd_)),timerfd_(createtimerfd(timetvl)),\
timerchannel_(new Channel(this->ep(),timerfd_))
{
    wakeupchannel_->setreadcallback(std::bind(&EventLoop::handlewakeup, this));
    wakeupchannel_->enablereading();//异步唤醒io队列中的发送线程

    timerchannel_->setreadcallback(std::bind(&EventLoop::handletimer, this));
    timerchannel_->enablereading();//定时唤醒事件循环清理空闲连接
}
EventLoop::~EventLoop()
{
    // delete ep_;
}
void EventLoop::run()
{
    threadid_=syscall(SYS_gettid);//获取线程id
    while(stop_==false)
    {
        std::vector<Channel*>channels=ep_->loop(-1);  //每一次都返回自己监听到的事件

        if(channels.size()==0)
        {
            epolltimeoutcallback_(this);
        }else{
            for(auto ch:channels)
            {
                ch->handleevent();     //处理返回的事件 
            }
        }
    }
}

Epoll* EventLoop::ep()
{
    return ep_.get();
}

void EventLoop::setepolltimeoutcallback(std::function<void(EventLoop*)> fn)
{
    epolltimeoutcallback_=fn;
}

void EventLoop::removechannel(Channel* ch)
{
    ep_->removechannel(ch);
}

void EventLoop::updatechannel(Channel* ch)
{
    ep_->updatechannel(ch);
}

bool EventLoop::isinloopthread()
{
    return threadid_==syscall(SYS_gettid);
}

void EventLoop::queueinloop(std::function<void()> fn)
{
    {
        std::lock_guard<std::mutex> gd(mutex_);
        taskqueue_.push(fn);
    }
    wakeup();
}

void EventLoop::wakeup()
{
    uint64_t val=1;
    write(wakeupfd_,&val,sizeof(val));
}
void EventLoop::handlewakeup() {
    uint64_t val;
    read(wakeupfd_, &val, sizeof(val));

    std::vector<std::function<void()>> tasks;  
    {
        std::lock_guard<std::mutex> gd(mutex_);
        while (!taskqueue_.empty()) {
            tasks.push_back(std::move(taskqueue_.front()));
            taskqueue_.pop();
        }
    }  // 此处锁自动释放

    for (auto& fn : tasks) {
        fn(); 
    }
}

void EventLoop::handletimer()
{
    struct itimerspec timeout; 
    memset(&timeout, 0, sizeof(struct itimerspec));
    timeout.it_value.tv_sec = timetvl_; 
    timeout.it_value.tv_nsec = 0;
    timerfd_settime(timerfd_, 0, &timeout, 0);

    if(mainloop_){

    }else{
            time_t now = time(0);  // 获取当前时间
            for (auto it = conmp_.begin(); it != conmp_.end();) {
                if(it->second->timeout(now,timeout_))
                {
                    int fd=it->first;
                    {
                        std::lock_guard<std::mutex> gd(mmutex_);
                        it=conmp_.erase(it);
                    }
                    timercallback_(fd);
                }else ++it;
            }
    }
}

void EventLoop::newconnection(spConnection conn)
{
    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conmp_[conn->fd()]=conn;
    }
}

void EventLoop::settimercallback(std::function<void(int)> fn)
{
    timercallback_=fn;
}

void EventLoop::stop()
{
    stop_=true;//仅仅写这个还得等下次闹钟唤醒循环
    wakeup();

}

std::mutex& EventLoop::mutex()
{
    return mmutex_;
}