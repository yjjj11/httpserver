#include "Socket.h"
#include <errno.h>
#include <fstream>
#include <iostream>  // 包含cout所需的头文件
#include <arpa/inet.h>  // 确保包含inet_addr等函数的头文件
#include <netinet/tcp.h>
using namespace std;
using namespace yazi::socket;

// 构造函数：创建socket
Socket::Socket() : m_ip(""), m_port(0), m_sockfd(0) {
    // 调用系统socket函数（加::前缀）
    m_sockfd = ::socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, IPPROTO_TCP);
    if (m_sockfd < 0) {
        cout << "create socket error: errno=" << errno << " errmsg=" << strerror(errno) << endl;
    } else {
        //cout << "create socket success! sockfd=" << m_sockfd << endl;
    }
}

// 带参数构造函数：使用已有的sockfd
Socket::Socket(int sockfd) : m_ip(""), m_port(0), m_sockfd(sockfd) {}

// 析构函数：关闭socket
Socket::~Socket() {
    close();  // 调用类的close成员函数
}

// 绑定IP和端口（成员函数）
bool Socket::bind(const InetAddress& servaddr) {
    
// 尝试调用系统 bind 函数，将套接字 fd_ 绑定到 servaddr 代表的地址
    if (::bind(m_sockfd, servaddr.addr(), sizeof(sockaddr)) < 0) { 
        cout<<"bind() failed\n";  // 打印系统错误信息（如权限不足、端口被占用等）
        close();  // 关闭套接字，释放资源
        exit(-1);  // 直接终止程序，退出码 -1 表示异常
    }
    m_ip = servaddr.ip();
    m_port = servaddr.port();
    return true;
}

// 监听连接（成员函数）
bool Socket::listen(int backlog) {
    // 调用系统listen函数（加::前缀）
    if (::listen(m_sockfd, backlog) !=0) {
        cout << "socket listen error: errno=" << errno << ", errmsg=" << strerror(errno) << endl;
        return false;
    } else {
        //cout << "socket listen success! backlog=" << backlog << endl;
    }
    return true;
}

// 发起连接（成员函数）
bool Socket::connect(const std::string &ip, int port) {
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;  // 协议族（原AF_INETi错误）

    sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());  // 目标IP
    sockaddr.sin_port = htons(port);                  // 目标端口

    // 调用系统connect函数（加::前缀）
    if (::connect(m_sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) != 0) {
        if(errno!=EINPROGRESS){
            cout << "socket connect error: errno=" << errno << ", errmsg=" << strerror(errno) << endl;
            cout << "connect failed" << endl;
            return false;
        }
    } 
    pollfd fds;
    fds.fd = m_sockfd;
    fds.events = POLLOUT;
    poll(&fds, 1, -1);  // 成功的一定是可写的
    if(fds.revents == POLLOUT) {
        cout << "connect ok" << endl;
    } else {
        cout << "connect failed" << endl;
    }

    m_ip = ip;
    m_port = port;
    return true;
}

// 接受连接（成员函数）
int Socket::accept(InetAddress & clientaddr) {
    sockaddr_in peeraddr; 
    socklen_t len = sizeof(peeraddr); 
    int clientfd = accept4(m_sockfd, (sockaddr*)&peeraddr, &len, SOCK_NONBLOCK); 

    // 将获取到的客户端地址信息设置到 clientaddr 对象中
    clientaddr.setaddr(peeraddr); 
    return clientfd; 
}

// 发送数据（成员函数）
int Socket::send(const char* buf, int len) {
    // 调用系统send函数（加::前缀）
    if(m_sockfd == 0) return 0;
    return ::send(m_sockfd, buf, len, 0);
}

bool Socket::send(void* buffer, const size_t size)
{
    if(m_sockfd == 0) return 0;  // connect failed

    if((::send(m_sockfd, buffer, size, 0)) < 0) return false;
    return true;
}

bool Socket::sendfile(const string & filename, const size_t size)
{
    // 以二进制方式打开文件
    ifstream fin(filename, ios::binary);
    if(fin.is_open() == false) {
        cout << "open file: " << filename << " failed" << endl;
        return false;
    }

    int onread = 0;
    int totalbytes = 0;
    char buffer[4096];

    while(true)
    {
        memset(&buffer, 0, sizeof(buffer));
        // 从文件读取数据
        if(size - totalbytes > 4096) onread = 4096;
        else onread = size - totalbytes;
        fin.read(buffer, onread);

        if(send(buffer, onread) == false) return false;

        totalbytes += onread;
        if(totalbytes == size) break;
    }
    fin.close();
    return true;
}   

bool Socket::recvfile(const string&  filename, const size_t size)
{
    ofstream fout;
    fout.open(filename, ios::binary);
    if(fout.is_open() == false) {
        cout << "open file failed:" << filename << endl;
        return false;
    }

    int totalbytes = 0;
    int onread = 0;
    char buffer[4096];

    while(true)
    {
        // 计算这次应该接收的字节数
        onread = min(size - totalbytes, (unsigned long)4096);

        if(recv(buffer, onread) == false) return false;

        // 将接收到的数据写入文件
        fout.write(buffer, onread);

        totalbytes += onread;
        if(totalbytes == size) break;
    }
    fout.close();
    return true;
}

// 接收数据（成员函数）
int Socket::recv(char* buf, int len) {
    // 调用系统recv函数（加::前缀）
    if(m_sockfd == 0) return 0;
    return ::recv(m_sockfd, buf, len, 0);
}

// 关闭socket（成员函数）
void Socket::close() {
    if (m_sockfd > 0) {
        ::close(m_sockfd);  // 调用系统close函数（加::前缀）
        m_sockfd = 0;
        //cout << "socket closed" << endl;
    }
}

bool Socket::set_send_buffer(int size)
{
    int buff_size = size;
    if(setsockopt(m_sockfd, SOL_SOCKET, SO_SNDBUF, &buff_size, sizeof(buff_size)) < 0)
    {
        cout << "socket set_send_buffer failed   error=" << errno << ",errmsg=" << strerror(errno) << endl;
        return false;
    }
    return true;
}

bool Socket::set_recv_buffer(int size)
{
    int buff_size = size;
    if(setsockopt(m_sockfd, SOL_SOCKET, SO_RCVBUF, &buff_size, sizeof(buff_size)) < 0)
    {
        cout << "socket set_recv_buffer failed   error=" << errno << ",errmsg=" << strerror(errno) << endl;
        return false;
    }
    return true;
}

bool Socket::set_linger(bool active, int seconds)
{
    struct linger l;
    memset(&l, 0, sizeof(l));

    l.l_onoff = active ? 1 : 0;
    l.l_linger = seconds;
    if(setsockopt(m_sockfd, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) < 0)
    {
        cout << "socket set_linger failed   error=" << errno << ",errmsg=" << strerror(errno) << endl;
        return false;
    }
    return true;
}

bool Socket::set_keepalive()
{
    int flag = 1;
    if(setsockopt(m_sockfd, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag)) < 0)
    {
        cout << "socket set_keepalive failed   error=" << errno << ",errmsg=" << strerror(errno) << endl;
        return false;
    }
    return true;
}

bool Socket::set_reuseaddr()
{
    int flag = 1;
    if(setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0)
    {
        cout << "socket set_reuseaddr failed   error=" << errno << ",errmsg=" << strerror(errno) << endl;
        return false;
    }
    return true;
}

int Socket::get_sockfd()
{
    return m_sockfd;
}
// 设置TCP无延迟选项（禁用Nagle算法）
bool Socket::set_nodelay() {
    int flag = 1;
    // TCP_NODELAY选项用于禁用Nagle算法，减少小数据包的延迟
    if (setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0) {
        cout << "socket set_nodelay failed   error=" << errno << ",errmsg=" << strerror(errno) << endl;
        return false;
    }
    return true;
}

// 设置端口复用选项
bool Socket::set_reuseport() {
    int flag = 1;
    // SO_REUSEPORT允许多个套接字绑定到同一端口（需要内核支持）
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag)) < 0) {
        cout << "socket set_reuseport failed   error=" << errno << ",errmsg=" << strerror(errno) << endl;
        return false;
    }
    return true;
}

void Socket::setipport(const string& ip,uint16_t port)
{
    m_ip=ip;
    m_port=port;
}