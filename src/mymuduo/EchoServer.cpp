#include"EchoServer.h"

EchoServer::EchoServer(const std::string &ip, const uint16_t port,int subthreadnum, \
int workthreadnum) : tcpserver_(ip, port,subthreadnum),threadpool_(workthreadnum,"WORKS")
{
    tcpserver_.setnewconnectioncb(std::bind(&EchoServer::HandleNewConnection, this, std::placeholders::_1));
    tcpserver_.setcloseconnectioncb(std::bind(&EchoServer::HandleClose, this, std::placeholders::_1));
    tcpserver_.seterrorconnectioncb(std::bind(&EchoServer::HandleError, this, std::placeholders::_1));
    // tcpserver_.setonmessagecb(std::bind(&EchoServer::HandleMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcpserver_.setsendcompletecb(std::bind(&EchoServer::HandleSendComplete, this, std::placeholders::_1));
    
}

EchoServer::~EchoServer()
{
}

void EchoServer::Start()
{
    tcpserver_.start();
}

void EchoServer::HandleNewConnection(spConnection conn)
{ 
    printf("%s accept client(fd=%d, ip=%s, port=%d) ok!\n", 
       Timestamp::now().tostring().c_str(),conn->fd(), conn->ip().c_str(), conn->port());
}

void EchoServer::HandleClose(spConnection conn)
{ 
    printf("%s Connection Close(fd=%d, ip=%s, port=%d)\n", 
       Timestamp::now().tostring().c_str(),conn->fd(), conn->ip().c_str(), conn->port());
}

void EchoServer::HandleError(spConnection conn)
{ 
    // 客户端的连接错误，在TcpServer类中回调此函数。
    printf("close client(fd=%d, ip=%s, port=%d) \n", 
             conn->fd(), conn->ip().c_str(), conn->port());
}

void EchoServer::HandleMessage(spConnection conn, std::string& message)
{ 
    if(threadpool_.size()==0)
    {
        //如果没有工作线程，在io中执行计算
        OnMessage(conn,message);
    }
    else threadpool_.addtask(bind(&EchoServer::OnMessage,this,conn,message));
}

void EchoServer::HandleSendComplete(spConnection conn)
{ 

}

void EchoServer::OnMessage(spConnection conn, std::string& message)
{
   // printf("%s message(eventfd=%d):%s\n",Timestamp::now().tostring().c_str(),conn->fd(),message.c_str());
    message="reply"+message;
    conn->send(message);
}

void EchoServer::Stop()
{
    //停止工作线程
    threadpool_.Stop();

    tcpserver_.stop();
    //停止io线程
}