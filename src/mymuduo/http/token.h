#pragma once
#include<unordered_map>
#include<iostream>
#include<string>
#include<chrono>
using namespace std;
class token{
private:
    unordered_map<string,pair<string,int>>store_;
    int64_t expire_time_=3600;
public:
    static token* getInstance();
    string generate_token(const string& id);
    bool check_token(const string& token,string& userid);
    bool update_expire_time(const string& token,int& expire_time);
    void delete_token(const string& token);
};
