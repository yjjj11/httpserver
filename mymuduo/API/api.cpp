#include"api.h"
#include"../http/token.h"
API::API()
{
    redis_=make_shared<redis::Redis>("tcp://127.0.0.1:6379");
    if(!redis_)error("redis_初始化失败");
    vector<string> keys;
    keys.emplace_back(all_images_key);
    keys.emplace_back(imagerank);
    if(checkRedisHealth(keys)) {
        cout << "Redis数据已从持久化恢复\n";
        loadToPathMapOnly();
    } else {
        cout << "执行全量数据加载\n";
        preloadImages();
    }
    manager_.rgister_function("/api/getRandomImage",getRandomImage);
    manager_.rgister_function("/api/getimagePrefer",getimagePrefer);
    manager_.rgister_function("/api/incrybyImagePrefer",incrybyImagePrefer);
    manager_.rgister_function("/api/getImageRanking",getImageRanking);
    manager_.rgister_function("/api/getUserFavoriteImages",getUserFavoriteImages);
    cout<<"API初始化完成\n";
}
bool API::checkRedisHealth(const vector<string>& keys)
{
    try {
        // 检查关键数据是否存在
        for(auto& key:keys){
            if(!redis_->exists(key)) {
                debug("Redis中无{}数据",key);
                return false;
            }
            
            auto count = redis_->hlen(key);
            if(count == 0) {
                debug("Redis中{}数据为空",key);
                return false;
            }
            cout<<"检查"<<key<<"成功\n";
        }
        
        // debug("Redis健康检查通过,有 {} 张图片", count);
        return true;
        
    } catch(const std::exception& e) {
        error("Redis健康检查失败: {}", e.what());
        return false;
    }
}
void API::loadToPathMapOnly()
{
    try {
        // 从 Redis 加载所有数据到 path_map_

        int count=stoi(*redis_->get("images_count"));
        path_map_.clear();
        for(int i=1;i<=count;i++)
        {
            path_map_[i]=*redis_->hget(to_string(i),"image_path");
            //cout<<i<<"对应图片 :"<<path_map_[i]<<"\n";
        }
        
        debug("从持久化Redis加载 {} 张图片到path_map_", count);
        cout << "从持久化Redis恢复 " << count << " 张图片\n";
        
    } catch(const exception& e) {
        error("从Redis加载path_map_失败: {}", e.what());
        // 失败时执行全量加载
        preloadImages();
    }
}
void API::preloadImages()
{
    cout<<"加载图片数据到redis\n";
    debug("加载图片数据到redis");

    auto mysql=get_one_connection();
    if(!mysql)debug("preloadImages连接mysql失败\n");


    char sql[1024]={0};
    snprintf(sql,sizeof(sql),"select id,image_name,prefer from images");
    if(mysql->query(sql))
    {
        int count=0;
        while(mysql->next())
        {
            string id=mysql->value(0);
            string image_name=mysql->value(1);
            string prefer=mysql->value(2);
            redis_->hset(all_images_key,id,"/images/"+image_name);
            redis_->hset(id,"image_path","/images/"+image_name);
            redis_->hset(id,"prefer",prefer);
            redis_->zadd(imagerank,id,stod(prefer));  
            count++;
            path_map_[stoi(id)]="/images/"+image_name;
        }
        redis_->set("images_count",to_string(count));
        debug("预备加载完成");
        cout<<"预加载完成\n";
        return;
    }
    cout<<"预加载失败\n";
    error("预加载失败");
}
shared_ptr<MysqlConn>  API::get_one_connection()
{
    ConnectionPool* pool = ConnectionPool::getConnectionPool();//拿到连接池
    if (!pool)
    {
        error("无法获取数据库连接池");
        cout<<"无法获取数据库连接池\n";
        return nullptr;
    }

    auto mysql=pool->getConnection();
    if (!mysql)
    {
        error("无法获取数据库连接");
        cout<<"无法获取数据库连接\n";
        return nullptr;
    }
    return mysql;
}

string API::getRandomImage()
{
    cout<<"进入了post_的函数筛选getRandomImage\n";
    
    if(!httprequest::post_.count("num"))return "0";
    string num=httprequest::post_.find("num")->second;

    cout<<"传入的num= "<<num<<"\n";
    return API::getInstance()->getRandomImageFromRedis(num);
}

string API::getRandomImageFromRedis(const string& num)
{   
    cout<<"进入了getRandomImageFromRedis\n";
    
    int id = stoi(num);
    // if(path_map_.find(id) != path_map_.end())
    // {
    //     debug("从path_map_获取图片ID{}: {}", id, path_map_[id]);
    //     cout<<"pathmap="<<path_map_[id]<<"\n";
    //     return path_map_[id];
    // }

    if(redis_){
        auto res = redis_->hget(num, "image_path");
        if(res)
        {
            debug("从Redis获取图片ID{}: {}", num, *res);
            path_map_[id] = *res;//更新内存操作
            return *res;
        }
        string path="resources/Json/image"+num+".json";
        ofstream ofs(path, ios::out);

        Json::Value root;
        root["url"]=*res;
        root["id"]=num;

        if(!ofs.is_open())
        {   
            cout<<"resources/Json/image"+num+".json未打开成功\n";
            return "0";
        }
        Json::StyledStreamWriter writer;
        writer.write(ofs,root);
        ofs.close();
    return  "/Json/image"+num+".json"; 
    }
    
    // 最后回退到数据库
    debug("缓存未命中,从数据库查询ID{}", num);
    return getRandomImageFromDB(num);
}

string API::getRandomImageFromDB(const string& num)
{
    cout<<"进入了getRandomImageFromDB\n";
    
    auto mysql=get_one_connection();
    if(!mysql)return "0";
    
    char sql[1024]={0};
    snprintf(sql,sizeof(sql),"select image_name from images where id=%s",num.c_str());
    if(mysql->query(sql) && mysql->next())
    {
        string image_path=mysql->value(0);
        debug("拿到的图片是{}",image_path);
        cout<<"拿到的图片是："<<image_path<<"\n";

        redis_->hset(all_images_key,num,"/images/"+image_path);//更新缓存，redis是网路+内存，比path_map_慢一点

        return "/images/"+image_path;
    }
    return num;
}


any API::call(const string& name)
{
    return manager_.call_function(name);
}

string API::getimagePrefer()
{
    cout<<"进入了getimagePrefer\n";
    debug("进入了getimagePrefer");
    string imageid;
    if(httprequest::post_.count("id"))
    {
        imageid=httprequest::post_.find("id")->second;
        cout<<"要查找的图片的id是"<<imageid<<"\n";
        string userid=get_userid();
        return API::getInstance()->getimagePreferFromRedis(userid,imageid);
    }else{
        cout<<"找不到那个id\n";
        return "0";
    }
    
}

string  API::getimagePreferFromRedis(const string& userid,const string& imageid)
{
    string prefer=*redis_->hget(imageid,"prefer");
    //如果找到了id，那么要返回他的prefer，可以先写进json文件中，在通过mmap发送统一管理
    string path="resources/Json/prefer"+userid+".json";
    ofstream ofs(path, ios::out);

    Json::Value root;
    root["prefer"]=prefer;

    if(!ofs.is_open())
    {   
        cout<<"resources/Json/prefer"+userid+".json未打开成功\n";
        return "0";
    }
    Json::StyledStreamWriter writer;
    writer.write(ofs,root);
    ofs.close();
    return  "/Json/prefer"+userid+".json"; 
}
string API::get_userid()
{
    string userid;
        string tokens;
        if(httprequest::header_.count("Cookie"))
        {
            tokens=httprequest::header_.find("Cookie")->second;
            auto pos=tokens.find("_");
            tokens=tokens.substr(0,pos);
        }
        int ret=token::getInstance()->check_token(tokens,userid);
        if(!ret)
        {
            cout<<"解析token登陆对象失败\n";
        }
        return tokens;
}
string API::incrybyImagePrefer()
{
    cout<<"进入了incrybyImagePrefer\n";
    string id;
    if(httprequest::post_.count("id"))
    {
        id=httprequest::post_.find("id")->second;
        cout<<"要查找的图片的id是"<<id<<"\n";
        string userid=get_userid();
        return API::getInstance()->incryRedisBy(userid,id,"prefer",1);
    }else{
        cout<<"找不到那个id\n";
        return "0";
    }
    
}
bool API::update_image_prefer_at_mysql(const string& userid,const string& id)
{
    auto mysql=get_one_connection();
    if(!mysql)
    {
        cout<<"mysql获取失败\n";
        return "0";
    }
    char sql[1024]={0};
    snprintf(sql,sizeof(sql),"update images set prefer=prefer+1 where id=%s",id.c_str());
    if(mysql->update(sql))
    {
        cout<<"更新总点赞成功\n";
    }
    //更新用户点赞
    

    snprintf(sql,sizeof(sql),"INSERT INTO user_image_likes (user_id, image_id, like_count)\
            VALUES (%s, %s, 1)\
            ON DUPLICATE KEY UPDATE\
            like_count = like_count + 1,\
            update_time = CURRENT_TIMESTAMP",userid.c_str(),id.c_str());

    if(mysql->update(sql))
    {
        cout<<"更新单用户点赞成功\n";
        return true;
    }
    return false;
}
string API::incryRedisBy(const string& userid,const string& imageid,const string& filed,const int& depth)
{
    
    if(update_image_prefer_at_mysql(userid,imageid)){//点赞更新数据库
        cout<<"更新点赞数据库成功\n";
    }
    string count;
    try{
        auto after_prefer=redis_->hincrby(imageid,"prefer",depth);
        count=to_string(after_prefer);
        auto after_prefer_user=redis_->hincrby("user"+userid+":"+imageid,"prefer",depth);
    }catch (const exception& e) {
        error("点赞失败(Redis操作);{}", e.what());
        return "0"; // 失败返回-1
    }

    //如果找到了id，那么要返回他的prefer，可以先写进json文件中，在通过mmap发送统一管理
    string path="resources/Json/prefer"+userid+".json";
    ofstream ofs(path, ios::out);

    Json::Value root;
    root["success"]="true";
    root["count"]=count;
    if(!ofs.is_open())
    {   
        cout<<"resources/Json"+userid+"/prefer.json未打开成功\n";
        return "0";
    }
    Json::StyledStreamWriter writer;
    writer.write(ofs,root);
    ofs.close();
    redis_->zincrby(imagerank,1.0,imageid);
    redis_->zincrby("user"+userid+":"+imagerank,1.0,imageid);
    return  "/Json/prefer"+userid+".json"; 
}
string API::getImageRanking()
{
    cout<<"进入了getImageRanking\n";
    string userid=get_userid();
    return API::getInstance()->getImageRankingFromRedis(userid);
}

string API::getImageRankingFromRedis(const string& userid)
{
    vector<pair<string,double>>rank;
    redis_->zrevrange(imagerank,0ll,3ll,back_inserter(rank));//提取前十名的
    
    Json::Value root(Json::arrayValue);
    for (const auto& item : rank)
    {
        Json::Value obj;
        obj["id"] = item.first;
        obj["url"]=*redis_->hget(item.first,"image_path");
        obj["count"] = to_string(item.second);
        root.append(obj);
        cout<<obj["url"]<<"\n";
    }
    StyledStreamWriter writer;
    string path="resources/Json/rank"+userid+".json";
    ofstream ofs(path, ios::out);
    if(!ofs.is_open())
    {   
        cout<<"resources/Json/rank"+userid+".json“未打开成功\n";
        return "0";
    }
    writer.write(ofs,root);
    ofs.close();
    return "/Json/rank"+userid+".json"; 
}

string API::getUserFavoriteImages()
{
    cout<<"进入了getImageRanking\n";
    string userid=get_userid();
    return API::getInstance()->getUserFavoriteImages_fromRedis(userid);
}

string API::getUserFavoriteImages_fromRedis(const string& userid)
{
    vector<pair<string,double>>rank;
    redis_->zrevrange("user"+userid+":"+imagerank,0ll,3ll,back_inserter(rank));//提取前十名的
    
    Json::Value root(Json::arrayValue);
    for (const auto& item : rank)
    {
        Json::Value obj;
        obj["id"] = item.first;
        obj["url"]=*redis_->hget(item.first,"image_path");
        obj["count"] = to_string(item.second);
        root.append(obj);
        cout<<obj["url"]<<"\n";
    }
    StyledStreamWriter writer;
    string path="resources/Json/rank"+userid+".json";
    ofstream ofs(path, ios::out);
    if(!ofs.is_open())
    {   
        cout<<"resources/Json/rank"+userid+".json“未打开成功\n";
        return "0";
    }
    writer.write(ofs,root);
    ofs.close();
    return "/Json/rank"+userid+".json"; 
}