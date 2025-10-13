#include"httprequest.h"
#include <regex>
#include"../API/api.h"
#include"token.h"

void httprequest::init()
{
    method_=vertion_=body_=path_=token_="";
    header_.clear();
    post_.clear();
    status_=request_line;//从头开始解析
    body_need=true;
}
bool httprequest::isAlive(){return isAlive_;}


bool httprequest::parse(string& buffer)
{
    debug("进入到了parse");
    string CRLF="\r\n";
    int read=0,ret=0;
    if(buffer.size()<=0)return true;
    notready_to_ready();
    if(status_==body)
    {
        parse_body(buffer,read);
    }

    while(buffer.size()>0 && status_!=finish)
    {
        int pos=buffer.find(CRLF,read); 
        
        if(pos==string::npos){
            ready_to_notready();
            debug("请求未完整到达");
      
            return true;
        }
        string line=buffer.substr(read,pos-read);
        //debug("read line: {}",line);
        switch(status_)
        {
            case request_line:
                ret=parse_line(line);
                if(!ret){
                    error("parse_line  error");
                    return false;
                }
                parse_path();
                debug("解析出的网页请求路径为:{}",path_);
                read=pos+2;
                break;
            case header:
                ret=parse_header(line);
                if(!ret){
                    error("parse_header  error");
                    return false;
                }
                read=pos+2;

                if(status_==body){
                    if(buffer.size()< stoi(header_["Content-Length"]))//处理请求体分包情况
                    {
                        ready_to_notready();
                        debug("body不完整，等待下一次读事件");
                       
                        return true;
                    }
                    ret=parse_body(buffer,read);
                    if(!ret){
                        error("parsebody error");
                       
                        return false;
                    }
                }
                break;
            default:
                break;
        }
    }
    buffer=buffer.substr(read);
    debug("parse over");
    debug("{}",method_);
    debug("{}",path_);
    debug("{}",vertion_);
    debug("{}",body_);
    return true;//这里返回true说明请求格式没有错误
}

void httprequest::notready_to_ready()
{
    if(status_==body_notready)status_=body;
    if(status_==line_notready)status_=request_line;
    if(status_==header_notready)status_=header;
}

void httprequest::ready_to_notready()
{
    if(status_==request_line)status_=line_notready;
    if(status_==header)status_=header_notready;
    if(status_==body)status_=body_notready;
}
bool httprequest::parse_line(string& line)
{
    debug("进入了parse_line");
    regex pattern("^([A-Z]+) (/[^ ]*) HTTP/(\\d+\\.\\d+)$");
    smatch submatch;
    if(regex_match(line, submatch, pattern)) {   
        method_ = submatch[1];
        path_ = submatch[2];
        vertion_ = submatch[3];
        status_ = header;
        return true;
    }
    status_ = header;
    error("RequestLine Error: {}" , line);  // 打印错误行，便于调试
    return false;
}

void httprequest::parse_path()
{
    if(path_=="/"){
        path_=path_+"index.html";
    }
    debug("进入了parse_path");
    for(auto& ch:default_html)
    {
        if(path_==ch)path_=path_+".html";
        break;
    }
    // path_=path_.substr(1);
}   

#include <algorithm>
bool httprequest::parse_header(string& line)
{
    //debug("进入了parse_header");
    if(line.empty())
    {
        debug("进入了： {}",line.empty());
        // 找到空行，判断是否有请求体
        if(header_.count("Content-Length"))  // 存在Content-Length头
        {
            debug("进入了： {}",header_.count("Content-Length"));
      
            // 尝试解析Content-Length的值
            try {
                int len = stoi(header_.find("Content-Length")->second);
                if(len > 0) {
                    status_ = body;  // 有请求体，进入body解析状态
                } else {
                    status_ = finish;  // 内容长度为0，解析完成
                }
            } catch(...) {
                error("invalid content-length value");
                return false;
            }
        } else {
            parse_API();
            status_ = finish;  // 没有Content-Length，解析完成
        }
        return true;
    }
    
    regex pattern("^([^:]+):\\s*(.*)$");
    smatch subMatch;
    if (regex_match(line, subMatch, pattern)) {
    std::string key = subMatch[1];
    std::string value = subMatch[2];

    value = trim(value);
    
    header_[key] = value;
    return true;
} else {
        error("Invalid header line: {}" , line);  // 打印错误行
        return false;
    }
}


// 移除字符串首尾的所有空白字符
std::string httprequest::trim(const std::string& s) {
    // 处理空字符串情况
    if (s.empty()) {
        return s;
    }

    // 找到第一个非空白字符的位置
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        start++;
    }

    // 整个字符串都是空白字符的情况
    if (start == s.size()) {
        return "";
    }

    // 找到最后一个非空白字符的位置
    size_t end = s.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end]))) {
        end--;
    }

    // 提取首尾非空白字符之间的子串
    return s.substr(start, end - start + 1);
}

bool httprequest::parse_body(string& buffer,int& read)
{
    debug("进入了parse_body");
    int len = stoi(header_.find("Content-Length")->second);//请求体长度
    body_=buffer.substr(read,len);
    parse_POST();
    read+=len;
    status_=finish;
    return true;
}

void httprequest::parse_POST()
{
    debug("进入到了parsepost函数");
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        int ret=parse_form_data();//将表单中的数据存储到键值对中
        if(!ret)debug("parse_from_data error");
        else debug("parse_from_data succesfull");
        if(choice_in.count(path_)) {//分辨是登陆还是注册
            int tag = choice_in.find(path_)->second;
            debug("Tag:%d", tag);
            if(tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if(isLogin)debug("处理登陆业务");
                else debug("处理注册业务");
                if(UserVerify(post_["username"], post_["password"], isLogin)) {
                    path_ = "/welcome.html";
                    debug("UserVerify Sucessiful");
                } 
                else {
                    path_ = "/error.html";
                    debug("UserVerify failed");
                }//根据验证是否成功来跳转
            }
        }
    }   
    parse_API();
}

bool httprequest::parse_form_data() {
        post_.clear();  // 清空之前的数据
        if (body_.empty()) return false;

        std::string key, value;
        int n = body_.size();
        int i = 0, j = 0;

        for (; i < n; ++i) {
            char ch = body_[i];
            switch (ch) {
                case '=':
                    // 提取键并解码
                    key = UrlDecode(body_.substr(j, i - j));
                    j = i + 1;  // 移动到值的起始位置
                    break;
                case '&':
                    // 提取值并解码，存入post_
                    value = UrlDecode(body_.substr(j, i - j));
                    post_[key] = value;
                    j = i + 1;  // 移动到下一个键的起始位置
                    break;
                default:
                    // 其他字符不处理，等待后续提取
                    break;
            }
        }
    
        // 处理最后一组键值对（没有&结尾的情况）
        if(j>i){
            error("最后一对键值对处理异常");
            return false;
        }
        if (j < i) {
            value = UrlDecode(body_.substr(j, i - j));
            post_[key] = value;
        }
    return true;
}


int httprequest::ConverHex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;  // 无效的十六进制字符
}

    // 解码URL编码的字符串（处理+和%XX）
    std::string httprequest::UrlDecode(const std::string& str) {
        std::string result;
        int n = str.size();
        for (int i = 0; i < n; ++i) {
            char ch = str[i];
            if (ch == '+') {
                // 处理空格的+表示法
                result += ' ';
            } else if (ch == '%' && i + 2 < n) {
                // 处理%XX编码
                int high = ConverHex(str[i+1]);
                int low = ConverHex(str[i+2]);
                if (high != -1 && low != -1) {
                    // 正确转换为原始字符（如%20→空格）
                    result += static_cast<char>(high * 16 + low);
                    i += 2;  // 跳过已处理的两个字符
                } else {
                    // 无效的编码，原样保留
                    result += ch;
                }
            } else {
                // 普通字符直接保留
                result += ch;
            }
        }
        return result;
}
bool httprequest::UserVerify(const string& username, const string& password, bool isLogin)
{
    if (username.empty() || password.empty())
    {
        error("用户名或密码为空");
        return false;
    }
    debug("Verify name:%s pwd:%s", username.c_str(), password.c_str());
    
    ConnectionPool* pool = ConnectionPool::getConnectionPool();
    if (!pool)
    {
        error("无法获取数据库连接池");

        return false;
    }
    
    auto mysql = pool->getConnection();
    if (!mysql)
    {
        error("无法获取数据库连接");
    
        return false;
    }
    
    bool flag = false;
    if (isLogin) // 登录检测逻辑
    {
        char sql[1024] = {0};
        snprintf(sql, sizeof(sql), "select id from users where username='%s' and password='%s'", 
                 username.c_str(), password.c_str());
        
        if (mysql->query(sql) && mysql->next())
        {
            // 获取count(*)结果并判断是否大于0
            string countStr = mysql->value(0);
            int count = atoi(countStr.c_str());
            if (count > 0)
            {
                flag = true;
                error("登陆成功");
                debug("登陆成功");
                token_=token::getInstance()->generate_token(countStr);
            }
            else
            {
                flag = false;
                error("用户名或密码错误");
                debug("用户名或密码错误");
            }
        }
        else
        {
            flag = false;
            error("登录查询失败");
            debug("登录查询失败");
        }
    }
    else // 注册逻辑
    {
        // 1. 先检测用户名是否已存在
        char checkSql[1024] = {0};
        snprintf(checkSql, sizeof(checkSql), "select count(*) from users where username='%s'", 
                 username.c_str());
        
        if (mysql->query(checkSql) && mysql->next())
        {
            string countStr = mysql->value(0);
            int count = atoi(countStr.c_str());
            
            if (count > 0)
            {
                // 用户名已存在，注册失败
                flag = false;
                error("用户名已存在");
                
            }
            else
            {
                // 2. 用户名不存在，执行注册插入
                char insertSql[1024] = {0};
                snprintf(insertSql, sizeof(insertSql), "insert into users(username, password) values('%s', '%s')", 
                         username.c_str(), password.c_str());
                
                if (mysql->update(insertSql))
                {
                    flag = true;
                    
                    debug("注册成功，插入成功");
                    memset(insertSql,sizeof(insertSql),0);
                    snprintf(insertSql, sizeof(insertSql), "select id from users where username='%s' and password='%s'", 
                            username.c_str(), password.c_str());
                    
                    if (mysql->query(insertSql) && mysql->next())
                    {
                        // 获取count(*)结果并判断是否大于0
                        string countStr = mysql->value(0);
                        int count = atoi(countStr.c_str());
                        if (count > 0)token_=token::getInstance()->generate_token(countStr);
                    }
                }
                else
                {
                    flag = false;
                    error("注册失败,update错误");
                 
                }
            }
        }
        else
        {
            flag = false;
            error("用户名检测失败");
         
        }
    }

    debug("UserVerify Sucessiful");
    
    return flag;
}

void httprequest::parse_API()
{
    debug("进入到了parse_API");
    if(API::getInstance()->funcs_.count(path_))//如果不是登陆或者注册业务
        //就去其他衍生api业务中去找
        {
            debug("进入了API函数寻找");
            string old_Path=path_;
            string type=API::getInstance()->funcs_.find(path_)->second;
            path_=any_cast<string>(API::getInstance()->call(path_,header_,post_));
            
            debug("httprequst得到的path是:{}",path_);
           
            if(path_=="0")
            {
                debug("API{}处理失败",old_Path);
                path_ = "/error.html";
                return;
            }         
            if(path_.find('.')==string::npos)
            {
                debug("不需要返回请求体");
             
                body_need=false;
            }   
        }
}