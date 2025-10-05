#pragma once
#include<iostream>
#include<string>
#include<unordered_map>
#include<sw/redis++/redis++.h>
#include"functionmap.h"
#include"../http/httprequest.h"
#include<jsoncpp/json/json.h>
#include<fstream>
using namespace std;
using namespace sw;
class httprequest;

class API{
public:
    unordered_map<string, string> funcs_ = {
        {"/api/getRandomImage", "string"}  ,
        {"/api/getimagePrefer", "int"},
        {"/api/incrybyImagePrefer","string"},
        {"/api/getImageRanking","getImageRanking"},
        {"/api/getUserFavoriteImages","getUserFavoriteImages"}
    };
    static API* getInstance()
    {
        static API instance;
        return &instance;
    }
    any call(const string& name);
private:
    API();
    static string getRandomImage();
    string getRandomImageFromDB(const string& num);
    string getRandomImageFromRedis(const string& num);
    shared_ptr<MysqlConn>  get_one_connection();
    bool checkRedisHealth(const vector<string>& keys);
    void loadToPathMapOnly();
    void preloadImages();
    static string getimagePrefer();
    string getimagePreferFromRedis(const string& userid,const string& imageid);
    static string incrybyImagePrefer();
    string incryRedisBy(const string& userid,const string& id,const string& filed,const int& depth);
    static string getImageRanking();
    string getImageRankingFromRedis(const string& userid);
    bool update_image_prefer_at_mysql(const string& userid,const string& id);
    static string get_userid();
    static string getUserFavoriteImages();
    string getUserFavoriteImages_fromRedis(const string& userid);
private:
    FunctionManager manager_;
    shared_ptr<redis::Redis> redis_;
    map<int,string>path_map_;
    string all_images_key="all_images";
    string imagerank="image_rank";
friend class httprequest;
};

