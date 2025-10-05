#include"token.h"
token* token::getInstance()
{
    static token instance;
    return &instance;
}
string token::generate_token(const string& id)
{   
    string token=id;
    auto randomint=rand();
    token=id+"_"+to_string(randomint);
    auto expire_time=chrono::duration_cast<chrono::seconds>(chrono\
        ::system_clock::now().time_since_epoch()).count()+expire_time_;

    store_[token]={id,expire_time};
    cout<<"创建token成功\n";
    return token;
}
bool token::check_token(const string& token,string& userid)
{
    if(store_.count(token))
    {
        cout<<"token存在,该解析出用户id和更新过期时间\n";
        auto& [id,expire_time]=store_.find(token)->second;
        userid=id;
        if(update_expire_time(token,expire_time))return true;
        return false;
    }
    else return false;
}
void token::delete_token(const string& token)
{
    store_.erase(token);
}
bool token::update_expire_time(const string& token,int& expire_time)
{
    auto now=chrono::duration_cast<chrono::seconds>(chrono\
        ::system_clock::now().time_since_epoch()).count();
    if(now>expire_time)
    {
        delete_token(token);
        return false;
    }
    else{
        expire_time=now+expire_time_;
        return true;
    }
}