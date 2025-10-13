#include"httpresponse.h"
using namespace std;
httpresponse::httpresponse() {
    code_ = -1;
    path_ = srcDir_ = "";
    isKeepAlive_ = false;
    mmFile_ = nullptr; 
    mmFileStat_ = { 0 };
};

void httpresponse::init(const string& srcDir, string& path, bool isKeepAlive, int code){
    if(srcDir==""){
        error("根目录不允许为空");
        return;
    }
    if(mmFile_) { UnmapFile(); }//先清理上一次的映射内容
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    srcDir_ = srcDir;
    mmFile_ = nullptr; 
    mmFileStat_ = { 0 };
}

void httpresponse::UnmapFile()
{
    if(mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}


void httpresponse::make_response(Buffer& buffer,bool body_need)
{
    if(!body_need)
    {
        debug("当前操作不用返回什么");
    }
    else if(stat((srcDir_+path_).data(),&mmFileStat_)||S_ISDIR(mmFileStat_.st_mode))
    {
        debug("找不到对应的文件");
        code_=404;//网页未找到
    }else if(!(mmFileStat_.st_mode & S_IROTH))
    {
        debug("其他用户没有读权限");
        code_=403;
    }
    else if(code_==-1)code_=200;
    
    debug("资源路径是{}",(srcDir_+path_).data());
    debug("当前的code是{}",code_);
    errorhtml();
    add_statusline(buffer);
    AddHeader_(buffer);
    if(body_need)add_content(buffer);
}

void httpresponse::errorhtml()
{
    if(code_html.count(code_)==1)
    {
        path_=code_html.find(code_)->second;
        debug("找到了errorhtml{}",path_);
        stat((srcDir_+path_).data(),&mmFileStat_);
    }
}

void httpresponse::add_statusline(Buffer& buffer)
{
    debug("进入了add_statusline");
    string status="";
    if(code_status.count(code_)==1)
    {
        status=code_status.find(code_)->second;
    }
    else{
        code_=400;
        status=code_status.find(400)->second;
    }
    buffer.Append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
}

void httpresponse::AddHeader_(Buffer& buff) {
    debug("进入了add_header");
    buff.Append("Connection: ");
    if(isKeepAlive_) {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=20, timeout=120\r\n");
    } else{
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + GetFileType() + "\r\n");
    if(token_!="")
    {
        string cookie = 
            token_ + "; "
            "Max-Age=86400; "
            "Path=/; "
            "HttpOnly; "
            "SameSite=Strict";
        buff.Append("Set-Cookie:"+cookie+"\r\n");
    }
}


string httpresponse::GetFileType() {
    /* 判断文件类型 */
    int pos=path_.find('.');
    if(pos == string::npos) {
        return "text/plain";
    }
    string suffix = path_.substr(pos);
    if(suffix_type.count(suffix) == 1) {
        return suffix_type.find(suffix)->second;
    }
    return "text/plain";
}

void httpresponse::add_content(Buffer& buff)
{
    debug("进入了add_content");
    int fd = open((srcDir_ + path_).data(), O_RDONLY);
    if(fd<0){
        ErrorContent(buff,"File NotFound");
        debug("fd打开失败{}",(srcDir_ + path_).data());
        return;
    }
    debug("file path {}", (srcDir_ + path_).data());
    int* mmRet = (int*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);//开启文件映射
    if(*mmRet == -1) {
        ErrorContent(buff, "File NotFound!");
        return; 
    }
    mmFile_ = (char*)mmRet;//管理映射
    debug("映射成功");
    close(fd);
    buff.Append("Content-length: " + to_string(mmFileStat_.st_size) + "\r\n\r\n");
}

void httpresponse::ErrorContent(Buffer& buff, string message) 
{
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(code_status.count(code_) == 1) {
        status = code_status.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += to_string(code_) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}