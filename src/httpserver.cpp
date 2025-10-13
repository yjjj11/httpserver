#include"httpserver.h"

HttpServer::HttpServer(const std::string &ip, const uint16_t port,int subthreadnum, \
int workthreadnum) : tcpserver_(ip, port,subthreadnum),threadpool_(workthreadnum,"WORKS"),\
connpool_(ConnectionPool::getConnectionPool()),globl_api_(API::getInstance())
{
    tcpserver_.setnewconnectioncb(std::bind(&HttpServer::HandleNewConnection, this, std::placeholders::_1));
    tcpserver_.setcloseconnectioncb(std::bind(&HttpServer::HandleClose, this, std::placeholders::_1));
    tcpserver_.seterrorconnectioncb(std::bind(&HttpServer::HandleError, this, std::placeholders::_1));
    tcpserver_.setonmessagecb(std::bind(&HttpServer::HandleMessage, this, std::placeholders::_1));
    tcpserver_.setsendcompletecb(std::bind(&HttpServer::HandleSendComplete, this, std::placeholders::_1));
    char path[64];
    if(getcwd(path,sizeof(path))!=nullptr)
    {
        srcDir_=string(path)+"/resources";
    }else 
    {
        debug("获取工作目录srcDir失败");
    }
    debug("httpserver 中的srcDir: {}",srcDir_);
}

HttpServer::~HttpServer()
{

}

void HttpServer::Start()
{
    tcpserver_.start();
}

void HttpServer::HandleNewConnection(spConnection conn)
{ 
    debug("{} accept client(fd={}, ip={}, port={}) ok!", 
       Timestamp::now().tostring(), conn->fd(), conn->ip(), conn->port());
}

void HttpServer::HandleClose(spConnection conn)
{ 
    debug("{} Connection Close(fd={}, ip={}, port={})", 
    Timestamp::now().tostring(), conn->fd(), conn->ip(), conn->port());
}

void HttpServer::HandleError(spConnection conn)
{ 
    // 客户端的连接错误，在TcpServer类中回调此函数。
    debug("{} Connection Close(fd={}, ip={}, port={})", 
        Timestamp::now().tostring(), conn->fd(), conn->ip(), conn->port());
}

void HttpServer::HandleMessage(spConnection conn)
{ 
    if(threadpool_.size()==0)
    {
        //如果没有工作线程，在io中执行计算
        OnMessage(conn);
    }
    else threadpool_.addtask(bind(&HttpServer::OnMessage,this,conn));//加入到工作线程中去处理读取，解析，响应，发送
}

void HttpServer::HandleSendComplete(spConnection conn)
{ 
    string path=conn->get_path();
    debug("发送 {}成功",path);
}

void HttpServer::OnMessage(spConnection conn) {
    debug("进入到工作线程中开始执行");
    int ret=conn->do_http_parse();
    string path=conn->get_path();
    if(ret==1)
    {
        debug("初始化好的response");

        conn->init_200_response(srcDir_,path,conn->get_alive());
    }else if(ret==-1)
    {
        //初始化不好的response
        debug("初始化不好的response");

        conn->init_400_response(srcDir_,path,false);

    }
    else{
        //继续触发读事件，等待下一轮的数据完全到达
    }
    conn->make_response();
}

void HttpServer::Stop()
{
    //停止工作线程
    threadpool_.Stop();

    tcpserver_.stop();
    //停止io线程
}

void HttpServer::do_http_response(spConnection conn,string& path)
{
    int fd=open(path.c_str(),O_RDONLY);
    if(fd==-1)//无法打开文件，内部错误
    {
        debug("文件不存在");
        not_found(conn);
        return ;
    }
    struct stat st;
    fstat(fd, &st);
    size_t size = st.st_size;
    if(size==0){
        not_found(conn);
        return ;
    }
    send_header(conn,size);

    //debug("开始发送：{}",Timestamp::now().tostring());
    send_content(conn,size,fd);
}

void HttpServer::send_content(spConnection conn,size_t size,int& fd)
{
    char* content=(char*)mmap(NULL,size,PROT_READ,MAP_PRIVATE,fd,0);
    close(fd);
    if(content==MAP_FAILED)
    {
        not_found(conn);
        return;
    }
    conn->send(string(content));
}


void HttpServer::send_header(spConnection conn,size_t size)
{
    debug("-----------------------------------reply-----------------------------------------");
    string main_header = "HTTP/1.0 200 OK\r\nServer: YJJ Server\r\nContent-Type: text/html\r\nConnection: Close\r\n";
    char tmp[64];
    int len=snprintf(tmp, 64, "Content-Length: %ld\r\n\r\n",size);
    main_header=main_header+string(tmp);
    if(len>0)conn->send(main_header);
    else debug("send_header error");
}



bool HttpServer::get_http_head(string& realpath,spConnection conn, vector<string>& message)
{
    if (message.empty() || message[0].empty()) {
        debug("无效请求：请求行为空");
        return false;
    }
    
    for(auto& hang:message) debug("red line: {}",hang);
    
    char method[64] = {0};  // 初始化避免垃圾值
    char url[256] = {0};
    int i = 0, j = 0;
    char path[256]={0};
    string& header = message[0];
    size_t header_len = header.size();
    
    if(isspace(header[0]))bad_request(conn);//第一行是第一个字是空的

    // 2. 解析 method（带边界检查）
    while (j < header_len && !isspace(header[j]) && i < sizeof(method)-1) {
        method[i++] = header[j++];
    }
    method[i] = '\0';
    debug("当前的方法是：{}",method);
    
    // 3. 正确比较 GET 方法（固定比较前3个字符）
    if (strncasecmp(method, "GET", 3) == 0) {
        debug("method 是 GET");
        
        // 4. 跳过空格（修正 j 自增逻辑）
        while (j < header_len && isspace(header[j])) {
            j++;  // 先判断是否为空格，再自增
        }
        
        // 5. 解析 url（带边界检查）
        i = 0;
        while (j < header_len && !isspace(header[j]) && i < sizeof(url)-1) {
            url[i++] = header[j++];
        }
        url[i] = '\0';
        string urls(url);
        int pos=urls.find("?");
        if(pos!=urls.npos)urls=urls.substr(0,pos);
        debug("url: {}",urls);

        sprintf(path,"./html_docs%s",urls.c_str());
        debug("path= {}",path);
        realpath=path;
        return true;
        //响应http请求
        
    } else {
        debug("请求格式有问题，出错了");
        unimplemented(conn);
        return false;
    }
}

void HttpServer::not_found(spConnection conn)
{
    string message="HTTP/1.0 404 NOT FOUND\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<HTML lang=\"zh-CN\">\r\n"
    "<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">\r\n"
    "<HEAD>\r\n"
    "<TITLE>NOT FOUND</TITLE>\r\n"
    "</HEAD>\r\n"
    "<BODY>\r\n"
    "    <P>文件不存在！\r\n"
    "    <P>The server could not fulfill your request because the resource specified is unavailable or nonexistent.\r\n"
    "</BODY>\r\n"
    "</HTML>";

    conn->send(message);
}


void HttpServer::inner_error(spConnection conn) {
    string message = "HTTP/1.0 500 Internal Server Error\r\n\
Content-Type: text/html\r\n\
\r\n\
<HTML>\r\n\
<HEAD>\r\n\
<TITLE>Inner Error</TITLE>\r\n\
</HEAD>\r\n\
<BODY>\r\n\
<P>服务器内部出错.</P>\r\n\
</BODY>\r\n\
</HTML>";

    conn->send(message);
}

void HttpServer::unimplemented(spConnection conn) {
    string message = "HTTP/1.0 501 Method Not Implemented\r\n"
                        "Content-Type: text/html\r\n"
                        "\r\n"
                        "<HTML>\r\n"
                        "<HEAD>\r\n"
                        "<TITLE>Method Not Implemented</TITLE>\r\n"
                        "</HEAD>\r\n"
                        "<BODY>\r\n"
                        "    <P>HTTP request method not supported.\r\n"
                        "</BODY>\r\n"
                        "</HTML>";

    conn->send(message);
}

void HttpServer::bad_request(spConnection conn) {
    string message= "HTTP/1.0 400 BAD REQUEST\r\n"
                        "Content-Type: text/html\r\n"
                        "\r\n"
                        "<HTML>\r\n"
                        "<HEAD>\r\n"
                        "<TITLE>BAD REQUEST</TITLE>\r\n"
                        "</HEAD>\r\n"
                        "<BODY>\r\n"
                        "    <P>Your browser sent a bad request! \r\n"

                        "</BODY>\r\n"
                        "</HTML>";

    conn->send(message);
}