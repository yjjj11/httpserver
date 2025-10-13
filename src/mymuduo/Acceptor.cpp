#include"Acceptor.h"

// 构造函数初始化（列表初始化 loop_ 成员）
Acceptor::Acceptor(EventLoop* loop, const std::string &ip, const uint16_t port)
: loop_(loop),
  servsock_(new ServerSocket(InetAddress(ip, port))),  // 初始化服务器socket
  acceptchannel_(loop->ep(), servsock_->fd())  // 初始化Channel
{
    acceptchannel_.setreadcallback(
        std::bind(&Acceptor::newconnection, this)); 
    acceptchannel_.enablereading();                       
    }

Acceptor::~Acceptor() 
{
    delete servsock_;
}

#include"Connection.h"

void Acceptor::newconnection()  // 用于处理新的连接请求
{
    while (true) 
    {
        InetAddress clientaddr;
        int clientfd = servsock_->accept(clientaddr);

        if (clientfd < 0) 
        {
            // 情况1：暂时无新连接（非阻塞socket的正常行为）
            if (errno == EAGAIN || errno == EWOULDBLOCK) 
            {
                break;  // 退出循环，处理其他事件
            }
            // 情况2：被信号中断（如Ctrl+C），重试accept
            else if (errno == EINTR) 
            {
                continue;
            }
            // 情况3：发生错误（如文件描述符耗尽、权限问题等）
            else 
            {
                cerr << "accept() error: " << strerror(errno) << endl;
                exit(-1);
            }
        }                 
        std::unique_ptr<Socket> clientsock(new Socket(clientfd));
        clientsock->setipport(clientaddr.ip(),clientaddr.port());
        // 成功接受新连接，初始化客户端Channel
        newconnectioncb_(std::move(clientsock));
        
    }
}

 void Acceptor::setnewconnection(std::function<void(std::unique_ptr<Socket>)> fn)
 {
    newconnectioncb_=fn;
 }