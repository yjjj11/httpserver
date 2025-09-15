#pragma once
#include<string>
#include <unordered_map>
#include <unordered_set>
#include <cctype>  // 包含isspace函数
#include"../log.h"
#include"../../DatabaseConnectionPool/ConnectionPool.h"
using namespace std;
class httprequest
{
public:
    enum parse_status{
        request_line,header,body,finish,body_notready,header_notready,line_notready
    };
    enum code_status{
        no_request=0,get_request,bad_request,no_resource,internal_error,
    };
    httprequest(){init();}
    ~httprequest()= default;

    bool isAlive();//接口
    bool parse(string& buffer);
    string getpath(){return path_;}
    string getmethod(){return method_;}
    string getversion(){return vertion_;}
    parse_status getstatus(){return status_;}
    bool get_alive(){return isAlive_;}
    void clean_last(){init();}
private:
    string trim(const std::string& s);
    void init();//函数内方法
    bool parse_line(string& line);
    bool parse_header(string & line);
    bool parse_body(string& buffer,int& read);
    void parse_path();
    void parse_POST();
    bool parse_form_data();
    bool UserVerify(const string& username,const string& password,bool isLogin);
    int ConverHex(char c);
    void notready_to_ready();
    void ready_to_notready();
    string UrlDecode(const string& str);

    string path_,method_,vertion_,body_;
    parse_status status_;
    code_status code_;
    unordered_set<string> default_html
    {
        "/index", "/register", "/login",
        "/welcome", "/video", "/picture",
    };
    unordered_map<string,int>choice_in
    {
        {"/login.html",1},{"/register.html",0}
    };
    unordered_map<string, string> header_;//存储请求体的键值对
    unordered_map<string, string> post_;//存储post表单中的键值对
    bool isAlive_;
};