#pragma once
#include<string>
#include<iostream>
#include <cstdint>
#include<cstring>
class Buffer
{
private:
    std::string buf_;
    int now_;
    uint64_t sep_;//分割府
public:
    Buffer(uint64_t sep=1);
    ~Buffer();
    int& now(){return now_;}
    uint64_t getsep(){return sep_;}
    std::string& getbuf(){return buf_;}
    void append(const char*data,size_t size);
    void appendwithsep(const char* data,size_t size);
    size_t size();
    const char* data();         //返回buf的首地址
    void clear();     
    void erase(size_t pos,size_t nn);  

    void consume() {
        if (now_ >= buf_.size()) {
            buf_.clear();
            now_ = 0;
        } else {
            buf_ = buf_.substr(now_);
            now_ = 0;
        }
    }
    int pickmessage(std::string& ss);
};