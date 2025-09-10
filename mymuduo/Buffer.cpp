#include"Buffer.h"

Buffer::Buffer(uint64_t sep):sep_(sep),now_(0)
{

}
Buffer::~Buffer()
{

}

void Buffer::append(const char* data,size_t size)
{
    buf_.append(data,size);
}
size_t Buffer::size()
{
    return buf_.size();
}
const char* Buffer::data()
{
    return buf_.data();
}
void Buffer::clear()
{
    buf_.clear();
}

void Buffer::erase(size_t pos,size_t nn)
{
    buf_.erase(pos,nn);
}

void Buffer::appendwithsep(const char* data,size_t size)
{
    if(sep_==0)
    {
        buf_.append(data,size);
    }else if(sep_==1)
    {
        buf_.append((char*)&size,4);
        buf_.append(data,size);
    }
    
}

int Buffer::pickmessage(std::string& ss)
{
    if(buf_.size()==0)return false;
    int len=0;
    if(sep_==0)
    {
        ss=buf_;
        buf_.clear();
    }
    else if(sep_==1)
    {
        int len;
        memcpy(&len,buf_.data(),4);
        if(buf_.size()<len+4)return false;
        ss=buf_.substr(4,len);
        buf_.erase(0,len+4);
    }


    return true;
}