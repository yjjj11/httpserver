#pragma once
#include"Socket.h"
#include"Channel.h"
#include"EventLoop.h"
#include"Acceptor.h"
#include"Connection.h"
#include<map>
#include"ThreadPoll.h"
#include<memory>
#include<mutex>
using namespace std;
class TcpServer
{
private:
unique_ptr<EventLoop> mainloop_;        //多线程情况下一个TcpServer类可以有多个事件循环
    Acceptor acceptor_;    //一个tcpserver只有一个acceptor
    std::map<int, spConnection> conmp_;
    vector<unique_ptr<EventLoop>> subloops_;
    int threadnum_;     //线程池的大小就是从事件循环的大小
    ThreadPool threadpool_;    //线程池
    std::mutex mmutex_;         //修改Connection红黑树的锁
    
    std::function<void(spConnection)> newconnectioncb_;    // 回调EchoServer::HandleNewConnection()。
    std::function<void(spConnection)> closeconnectioncb_;  // 回调EchoServer::HandleClose()。
    std::function<void(spConnection)> errorconnectioncb_;  // 回调EchoServer::HandleError()。
    std::function<void(spConnection)> onmessagecb_;  // 回调EchoServer::HandleMessage()。
    std::function<void(spConnection)> sendcompletecb_;     // 回调EchoServer::HandleSendComplete()。
    std::function<void(EventLoop*)> timeoutcb_;           // 回调EchoServer::HandleTimeOut()。
public:
    TcpServer(const std::string &ip , const uint16_t port,int threadnum=3);
    ~TcpServer();

    void start();
    void stop();
    void newconnection(std::unique_ptr<Socket> clientsock);
    void closeconnection(spConnection conn); // 关闭客户端的连接，在Connection类中回调此函数。
    void errorconnection(spConnection conn); // 客户端的连接错误，在Connection类中回调此函数。
    void onmessage(spConnection conn);
    void sendcomplete(spConnection conn);
    void epolltimeout(EventLoop* loop);

    void setnewconnectioncb(std::function<void(spConnection)> fn);
    void setcloseconnectioncb(std::function<void(spConnection)> fn);
    void seterrorconnectioncb(std::function<void(spConnection)> fn);
    void setonmessagecb(std::function<void(spConnection)> fn);
    void setsendcompletecb(std::function<void(spConnection)> fn);
    void settimeoutcb(std::function<void(EventLoop*)> fn);
    void removeconn(int fd);
    void readmessage();
};