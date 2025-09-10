#include"Connection.h"

Connection::Connection(EventLoop* loop, std::unique_ptr<Socket>clientsock)
:clientsock_(std::move(clientsock)),loop_(loop),disconnect_(false),\
clientchannel_(new Channel(loop->ep(), clientsock_->fd()))
{
    clientchannel_->enablereading();  // 注册读事件
    clientchannel_->useet();          // 使用边缘触发模式
    clientchannel_->setreadcallback(bind(&Connection::onmessage,this));
    clientchannel_->setclosecallback(bind(&Connection::closecallback,this));
    clientchannel_->seterrorcallback(bind(&Connection::errorcallback,this));
    clientchannel_->setwritecallback(bind(&Connection::writecallback,this));
}

Connection::~Connection()
{

}

void Connection::closecallback()  // TCP连接关闭（断开）的回调函数，供Channel回调。
{
    disconnect_=true;
    closecallback_(shared_from_this());  // 使用shared_from_this获取自身的shared_ptr
}

void Connection::errorcallback()  // TCP连接错误的回调函数，供Channel回调。
{
    disconnect_=true;
    errorcallback_(shared_from_this());  // 使用shared_from_this获取自身的shared_ptr
}

void Connection::setclosecallback(std::function<void(spConnection)> fn)  // TCP连接关闭（断开）的回调函数，供Channel回调。
{
    closecallback_ = fn;
}

void Connection::seterrorcallback(std::function<void(spConnection)> fn)  // TCP连接错误的回调函数，供Channel回调。
{
    errorcallback_ = fn;
}

void Connection::onmessage()
{
    char buffer[1024];
    
    // 循环读取所有可用数据（非阻塞模式下）
    while (true) 
    { 
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = read(fd(), buffer, sizeof(buffer));
         printf("读取到数据: %.*s\n", (int)nread, buffer);  // 打印原始数据
        if (nread > 0)  // 成功读取到数据
        {
            inputbuffer_.append(buffer,nread);
        } 
        // 读取被信号中断，继续尝试读取
        else if (nread == -1 && errno == EINTR) 
        { 
            continue;
        } 
        // 非阻塞IO下数据已读完（无更多数据）
        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) 
        { 
            string line;
            int ret;
            while ((ret = getline(line)) == 1) {
                // 处理一行数据（请求行或头部）
                messages.push_back(line);
                printf("解析到行: %s\n", line.c_str());
            }

            if (ret == -1) {
                // 读到空行，HTTP头部结束
                lasttime_ = Timestamp::now();
                inputbuffer_.consume();
                //printf("检测到空行，头部解析完成\n");  // 确认是否走到这里
                //将当前报文传输并计算
                onmessagecallback_(shared_from_this(),messages);
                messages.clear();
            }
            break;
            
        } 
        // 客户端主动断开连接（读返回0）
        else if (nread == 0) 
        { 
            closecallback();
            clientchannel_->remove();
            break;
        }
    }
}
void Connection::setonmessagecallback(std::function<void(spConnection, vector<string>&)> fn)
{
    onmessagecallback_ = fn;
}

// 发送数据，不管在任何线程中，都是调用此函数发送数据。
// 重载两个版本，分别处理左值和右值，避免不必要的拷贝
void Connection::send(string& data)  // 处理左值（const引用，不会修改原数据）
{
    if (disconnect_) {
        printf("客户端连接已断开了，send()直接返回。\n");
        return;
    }
    // 对于左值，只能拷贝（因为不能修改原数据）
    auto message = std::make_shared<string>(move(data));  // 拷贝构造
    sendToLoop(message);  // 提取公共逻辑到sendToLoop
}

void Connection::send(string&& data)  // 处理右值（临时对象或std::move的对象）
{
    if (disconnect_) {
        printf("客户端连接已断开了，send()直接返回。\n");
        return;
    }
    // 对于右值，直接移动构造，无拷贝
    auto message = std::make_shared<string>(std::move(data));
    sendToLoop(message);
}

// 提取公共逻辑，避免代码重复
void Connection::sendToLoop(std::shared_ptr<string> message)
{
    if (loop_->isinloopthread()) {
        sendInloop(message);
    } else {
        std::lock_guard<std::mutex> gd(loop_->mutex());
        loop_->queueinloop(std::bind(&Connection::sendInloop, this, message));
    }
}

// 发送数据，如果当前线程是IO线程，直接调用此函数，如果是工作线程，将把此函数传给IO线程去执行。
void Connection::sendInloop(const std::shared_ptr<std::string>&data)
{
    //outputbuffer_.appendwithsep(data->data(), data->size());  // 把需要发送的数据保存到Connection的发送缓冲区中。
    outputbuffer_.append(data->data(),data->size());
    clientchannel_->enablewriting();  // 注册写事件。
}
void Connection::writecallback()
{
    if (outputbuffer_.size() == 0) {
        clientchannel_->disablewriting();
        return;
    }
    //尝试将缓冲区中的内容全部发送出去
    const char* buf = outputbuffer_.data();
    size_t buf_len = outputbuffer_.size();
    int writen = ::send(fd(), buf, buf_len, 0);
    
    if (writen > 0){
        //cout<<"发送了：\n"<<outputbuffer_.data()<<"  \n长度为 :"<<writen<<"\n";
        outputbuffer_.erase(0, writen);
        //cout<<"发送结束："<<Timestamp::now().tostring()<<"\n";
        if (outputbuffer_.size() == 0)
        {
            clientchannel_->disablewriting();
            sendcompletecallback_(shared_from_this());  // 使用shared_from_this获取自身的shared_ptr
        }
    }else if (writen == 0) {
        // 发送0字节通常表示连接关闭
        printf("连接已关闭，发送0字节\n");
    } else {
        // 区分临时错误（如EAGAIN）和致命错误
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            cout << "发送失败，错误errno: " << strerror(errno) << endl;
            // 致命错误时应关闭连接
        }
    }
}

void Connection::setsendcompletecallback(std::function<void(spConnection)> fn)
{
    sendcompletecallback_ = fn;
}

bool Connection::timeout(time_t now,int time)
{
    return now-lasttime_.toint()>time;
}


// 返回值：1=读到完整行，0=未读完，-1=空行
int Connection::getline(string& line) {
    string& buf = inputbuffer_.getbuf();
    int& now = inputbuffer_.now();
    size_t pos = buf.find("\r\n", now);  // 直接找\r\n作为行结束

    if (pos == string::npos) {
        return 0;  // 没找到完整行，等更多数据
    }

    // 提取行内容（不含\r\n）
    line = buf.substr(now, pos - now);
    now = pos + 2;  // 跳过\r\n

    return line.empty() ? -1 : 1;  // 空行返回-1，否则返回1

}