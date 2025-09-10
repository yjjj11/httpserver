#include"httpserver.h"

HttpServer::HttpServer(const std::string &ip, const uint16_t port,int subthreadnum, \
int workthreadnum) : tcpserver_(ip, port,subthreadnum),threadpool_(workthreadnum,"WORKS")
{
    tcpserver_.setnewconnectioncb(std::bind(&HttpServer::HandleNewConnection, this, std::placeholders::_1));
    tcpserver_.setcloseconnectioncb(std::bind(&HttpServer::HandleClose, this, std::placeholders::_1));
    tcpserver_.seterrorconnectioncb(std::bind(&HttpServer::HandleError, this, std::placeholders::_1));
    tcpserver_.setonmessagecb(std::bind(&HttpServer::HandleMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcpserver_.setsendcompletecb(std::bind(&HttpServer::HandleSendComplete, this, std::placeholders::_1));
    
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
    printf("%s accept client(fd=%d, ip=%s, port=%d) ok!\n", 
       Timestamp::now().tostring().c_str(),conn->fd(), conn->ip().c_str(), conn->port());
}

void HttpServer::HandleClose(spConnection conn)
{ 
    printf("%s Connection Close(fd=%d, ip=%s, port=%d)\n", 
       Timestamp::now().tostring().c_str(),conn->fd(), conn->ip().c_str(), conn->port());
}

void HttpServer::HandleError(spConnection conn)
{ 
    // 客户端的连接错误，在TcpServer类中回调此函数。
    printf("close client(fd=%d, ip=%s, port=%d) \n", 
             conn->fd(), conn->ip().c_str(), conn->port());
}

void HttpServer::HandleMessage(spConnection conn,  vector<string>& message)
{ 
    if(threadpool_.size()==0)
    {
        //如果没有工作线程，在io中执行计算
        OnMessage(conn,message);
    }
    else threadpool_.addtask(bind(&HttpServer::OnMessage,this,conn,message));
}

void HttpServer::HandleSendComplete(spConnection conn)
{ 

}

void HttpServer::OnMessage(spConnection conn, vector<string>& message) {
    // 1. 先检查消息合法性并解析
    string path;
    int ret=get_http_head(path,conn,message);
    //回应请求
    if(ret)
    {
        struct stat st;
        if(stat(path.c_str(),&st)==-1)//文件不存在或是出错
        {
            //响应404
            not_found(conn);
        }
        if(S_ISDIR(st.st_mode))path+="/index.html";//如果 是目录就跳转到那个首页
        do_http_response(conn,path);

    }
    
    
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
        cout<<"文件不存在\n";
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

    //cout<<"开始发送："<<Timestamp::now().tostring()<<"\n";
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
    cout<<"-----------------------------------reply-----------------------------------------\n";
    string main_header = "HTTP/1.0 200 OK\r\nServer: YJJ Server\r\nContent-Type: text/html\r\nConnection: Close\r\n";
    char tmp[64];
    int len=snprintf(tmp, 64, "Content-Length: %ld\r\n\r\n",size);
    main_header=main_header+string(tmp);
    if(len>0)conn->send(main_header);
    else cout<<"send_header error\n";
}



bool HttpServer::get_http_head(string& realpath,spConnection conn, vector<string>& message)
{
    if (message.empty() || message[0].empty()) {
        cout << "无效请求：请求行为空" << endl;
        return false;
    }
    
    for(auto& hang:message) cout<<"red line: "<<hang<<"\n";
    
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
    cout << "当前的方法是：" << method << endl;
    
    // 3. 正确比较 GET 方法（固定比较前3个字符）
    if (strncasecmp(method, "GET", 3) == 0) {
        cout << "method 是 GET" << endl;
        
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
        cout << "url: " << urls << endl;

        sprintf(path,"./html_docs%s",urls.c_str());
        cout<<"path= "<<path<<"\n";
        realpath=path;
        return true;
        //响应http请求
        
    } else {
        cout << "请求格式有问题，出错了" << endl;
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