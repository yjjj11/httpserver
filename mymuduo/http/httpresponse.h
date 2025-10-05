#pragma once
#include<string>
#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap
#include"../log.h"
#include<unistd.h>
#include<unordered_map>
#include"../Buffer.h"
using namespace std;
class httpresponse
{
public:
    unordered_map<int,string> code_status
    {
        {200,"ok"},
        {400,"Bad Request"},
        {403,"Forbidden"},
        {404,"Not Found"},
    };
    unordered_map<int,string> code_html
    {
        {400,"/400.html"},
        {403,"/403.html"},
        {404,"/404.html"},
    };
    unordered_map<string,string> suffix_type
    {
        { ".html",  "text/html" },
        { ".xml",   "text/xml" },
        { ".xhtml", "application/xhtml+xml" },
        { ".txt",   "text/plain" },
        { ".rtf",   "application/rtf" },
        { ".pdf",   "application/pdf" },
        { ".word",  "application/nsword" },
        { ".png",   "image/png" },
        { ".gif",   "image/gif" },
        { ".jpg",   "image/jpeg" },
        { ".jpeg",  "image/jpeg" },
        { ".au",    "audio/basic" },
        { ".mpeg",  "video/mpeg" },
        { ".mpg",   "video/mpeg" },
        { ".avi",   "video/x-msvideo" },
        { ".gz",    "application/x-gzip" },
        { ".tar",   "application/x-tar" },
        { ".css",   "text/css "},
        { ".js",    "text/javascript "},
        { ".json", "application/json" },
    };

public:
    httpresponse();
    ~httpresponse() =default;
    void init(const string& srcDir, string& path, bool isKeepAlive, int code);
    void make_response(Buffer& buffer,bool body_need);
    size_t fileLen(){return mmFileStat_.st_size;}
    char* file(){return mmFile_;}
    void set_token(const string& token){token_=token;}
private:
    string path_,token_;
    int code_;
    bool isKeepAlive_;
    string srcDir_;
    char* mmFile_; 
    struct stat mmFileStat_;
    string GetFileType();
    void AddHeader_(Buffer& buffer);
    void UnmapFile();
    void errorhtml();
    void add_content(Buffer& buffer);
    void add_statusline(Buffer& buffer);
    void ErrorContent(Buffer& buff, string message);
};