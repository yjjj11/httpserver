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
        debug("读取到数据:\n {}", buffer);  // 打印原始数据
        if (nread > 0)  // 成功读取到数据
        {
            inputbuffer_.append(buffer,nread);//为了不破坏封装
        } 
        // 读取被信号中断，继续尝试读取
        else if (nread == -1 && errno == EINTR) 
        { 
            continue;
        } 
        // 非阻塞IO下数据已读完（无更多数据）
        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) 
        {
                lasttime_ = Timestamp::now();
                debug("onread_");
                onmessagecallback_(shared_from_this());
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
void Connection::setonmessagecallback(std::function<void(spConnection)> fn)
{
    onmessagecallback_ = fn;
}

// 发送数据，不管在任何线程中，都是调用此函数发送数据。
// 重载两个版本，分别处理左值和右值，避免不必要的拷贝
void Connection::send(string& data)  // 处理左值（const引用，不会修改原数据）
{
    if (disconnect_) {
        debug("客户端连接已断开了，send()直接返回。");
        return;
    }
    // 对于左值，只能拷贝（因为不能修改原数据）
    auto message = std::make_shared<string>(move(data));  // 拷贝构造
    sendToLoop(message);  // 提取公共逻辑到sendToLoop
}

void Connection::send(string&& data)  // 处理右值（临时对象或std::move的对象）
{
    if (disconnect_) {
        debug("客户端连接已断开了，send()直接返回。");
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
    debug("开始发送数据了");
    int saveError;
    int ret=write(&saveError);
     if (ret < 0) {
        // 区分临时错误和致命错误
        if (saveError != EAGAIN && saveError != EWOULDBLOCK) {
            debug("发送失败: {}", strerror(saveError));
        }
        return;  // 临时错误等待下次写事件
    }

    // 发送成功后检查缓冲区是否为空
    bool isComplete = false;
    if (iovCnt_ == 1) {
        isComplete = (outputbuffer_.size() == 0);
    } else {
        isComplete = (iov_[0].iov_len + iov_[1].iov_len == 0);
    }

    if (isComplete) {
        // 发送完成：关闭写事件，触发回调
        sendcomplete();
    } else {
        // 未发送完成：继续等待写事件（下次内核缓冲区可用时再发）
        clientchannel_->enablewriting();
    }

}
void Connection::sendcomplete()
{
    clientchannel_->disablewriting();  // 缓冲区空，关闭写事件
    sendcompletecallback_(shared_from_this());  // 触发发送完成回调
    debug("发送完成");
    debug("");
    request_.clean_last();
}
    // //尝试将缓冲区中的内容全部发送出去
    // const char* buf = outputbuffer_.data();
    // size_t buf_len = outputbuffer_.size();
    // int writen = ::send(fd(), buf, buf_len, 0);
    
    // if (writen > 0){
    //     //debug("发送了：\n{}  \n长度为 :{}",outputbuffer_.data(),writen);
    //     outputbuffer_.erase(0, writen);
    //     //debug("发送结束：{}",Timestamp::now().tostring());
    //     if (outputbuffer_.size() == 0)
    //     {
    //         clientchannel_->disablewriting();
    //         sendcompletecallback_(shared_from_this());  // 使用shared_from_this获取自身的shared_ptr
    //     }
    // }else if (writen == 0) {
    //     // 发送0字节通常表示连接关闭
    //     debug("连接已关闭，发送0字节");
    // } else {
    //     // 区分临时错误（如EAGAIN）和致命错误
    //     if (errno != EAGAIN && errno != EWOULDBLOCK) {
    //         debug("发送失败，错误errno: {}", strerror(errno));
    //         // 致命错误时应关闭连接
    //     }
    // }
    

ssize_t Connection::write(int* saveErrno)
{
    ssize_t len =writev(fd(), iov_, iovCnt_);
        if(len <= 0) {
            *saveErrno = errno;
            return len;
        }
        if(static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            outputbuffer_.consumeAll();
            iov_[0].iov_len = 0;
        }
        else {
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
            iov_[0].iov_len -= len; 
            outputbuffer_.consumeLen(len);
        }
    return len;
}
void Connection::setsendcompletecallback(std::function<void(spConnection)> fn)
{
    sendcompletecallback_ = fn;
}

bool Connection::timeout(time_t now,int time)
{
    return now-lasttime_.toint()>time;
}


int  Connection::do_http_parse()
{
    debug("进入do_http_parse");
    string& buffer=inputbuffer_.getbuf();
    bool ret=request_.parse(buffer);
    token_=request_.get_token();
    response_.set_token(token_);
    if(ret==false){
        //说明在解析时，出现了问题，是一个bad_request;
        code_=httprequest::bad_request;
        return -1;
    }else{
        if(request_.getstatus()==httprequest::finish)//说明读取到了完整的请求
        {
        
            return 1;
        }
    }
    return 0;
}


void Connection::init_400_response(const string& srcDir,  string& path, bool isKeepAlive)
{
    response_.init(srcDir,path,isKeepAlive,400);
}

void Connection::init_200_response(const string& srcDir,  string& path, bool isKeepAlive)
{
    response_.init(srcDir,path,isKeepAlive,200);
}

void Connection::make_response()
{
    bool body_need=request_.get_body_need();
    response_.make_response(outputbuffer_,body_need);
    debug("发出去的请求\n{}",outputbuffer_.getbuf().c_str());
    iov_[0].iov_base=(char*)outputbuffer_.getbuf().c_str();
    iov_[0].iov_len=outputbuffer_.size();
    iovCnt_=1;

    if(body_need){
        if(response_.fileLen()>0 && response_.file())
        {
            iov_[1].iov_base=response_.file();
            iov_[1].iov_len=response_.fileLen();
            iovCnt_=2;
        }
        debug("文件已经映射完成,发送缓冲区中的行和头部也已经填充,准备发送,iovCnt:{}",iovCnt_);
    }
    clientchannel_->enablewriting();
}