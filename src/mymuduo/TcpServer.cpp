#include"TcpServer.h"
TcpServer::TcpServer(const std::string &ip , const uint16_t port,int threadnum)
:threadnum_(threadnum),mainloop_(new EventLoop(true)),acceptor_(mainloop_.get(),ip,port),\
threadpool_(threadnum_,"IO") //创建线程池
{
        //创建主事件循环
    mainloop_->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout,this,std::placeholders::_1));
    acceptor_.setnewconnection(std::bind(&TcpServer::newconnection,this,std::placeholders::_1));

    for(int i=0;i<threadnum_;i++)
    {
        subloops_.emplace_back(new EventLoop(false,5,10));//创建从事件循环并且为每一个从时间设置超时回调函数
        subloops_[i]->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout,this,std::placeholders::_1));
        subloops_[i]->settimercallback(std::bind(&TcpServer::removeconn,this,std::placeholders::_1));
        threadpool_.addtask(bind(&EventLoop::run,subloops_[i].get()));    //在线程池中运行从事件循环
    }
}

TcpServer::~TcpServer()
{
}   

void TcpServer::start()
{
    mainloop_->run();
}

void TcpServer::newconnection(std::unique_ptr<Socket> clientsock)
{
    // 创建Connection智能指针
    spConnection conn = std::make_shared<Connection>(subloops_[clientsock->fd()%threadnum_].get(), std::move(clientsock));
    conn->setclosecallback(std::bind(&TcpServer::closeconnection,this,std::placeholders::_1));
    conn->seterrorcallback(std::bind(&TcpServer::errorconnection,this,std::placeholders::_1));
    conn->setonmessagecallback(std::bind(&TcpServer::onmessage,this,std::placeholders::_1));
    conn->setsendcompletecallback(std::bind(&TcpServer::sendcomplete,this,std::placeholders::_1));
    
    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conmp_[conn->fd()] = conn;  // 存入map管理
    }
    subloops_[conn->fd()%threadnum_]->newconnection(conn);
    //触发新连接回调
    if (newconnectioncb_)
    {
        newconnectioncb_(conn);
    }
}

// 关闭客户端的连接，在Connection类中回调此函数。
void TcpServer::closeconnection(spConnection conn)
{
    // 触发关闭连接回调
    if (closeconnectioncb_)
    {
        closeconnectioncb_(conn);
    }

    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conmp_.erase(conn->fd());
    }
}

// 客户端的连接错误，在Connection类中回调此函数。
void TcpServer::errorconnection(spConnection conn)
{
    // 触发错误回调
    if (errorconnectioncb_)
    {
        errorconnectioncb_(conn);
    }

    {
         std::lock_guard<std::mutex> gd(mmutex_);
         conmp_.erase(conn->fd());  // 从map中移除，智能指针自动管理生命周期
    }    
}

void TcpServer::onmessage(spConnection conn)
{
    // 触发消息处理回调
    if (onmessagecb_)
    {
        onmessagecb_(conn);
    }
}

void TcpServer::sendcomplete(spConnection conn)
{
    // printf("send complete.\n");
    // 触发发送完成回调
    if (sendcompletecb_)
    {
        sendcompletecb_(conn);
    }
}

void TcpServer::epolltimeout(EventLoop* loop)
{
    printf("epoll_wait() timeout.\n");
    // 触发超时回调
    if(timeoutcb_)
    {
        timeoutcb_(loop);
    }
}   

// 设置新连接回调函数
void TcpServer::setnewconnectioncb(std::function<void(spConnection)> fn) {
    newconnectioncb_ = fn;
}

// 设置连接关闭回调函数
void TcpServer::setcloseconnectioncb(std::function<void(spConnection)> fn) {
    closeconnectioncb_ = fn;
}

// 设置连接错误回调函数
void TcpServer::seterrorconnectioncb(std::function<void(spConnection)> fn) {
    errorconnectioncb_ = fn;
}

// 设置消息处理回调函数
void TcpServer::setonmessagecb(std::function<void(spConnection)> fn) {
    onmessagecb_ = fn;
}

// 设置发送完成回调函数
void TcpServer::setsendcompletecb(std::function<void(spConnection)> fn) {
    sendcompletecb_ = fn;
}

// 设置超时回调函数
void TcpServer::settimeoutcb(std::function<void(EventLoop*)> fn) {
    timeoutcb_ = fn;
}

void TcpServer::removeconn(int fd)
{
    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conmp_.erase(fd);
    }
   
}

void TcpServer::stop()
{
    //停止事件循环
    mainloop_->stop();
    // cout<<"主事件停止\n";

    for(int i=0;i<threadnum_;i++)
        subloops_[i]->stop();
    // cout<<"从事件停止\n";
    threadpool_.Stop();
    // cout<<"线程池停止\n";

    //清理所有Connections;
}
