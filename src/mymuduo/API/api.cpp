#include"api.h"
#include"../http/token.h"
API::API()
{
    // Redis初始化和数据加载逻辑保持不变
    redis_ = make_shared<redis::Redis>("tcp://127.0.0.1:6379");
    if(!redis_) error("redis_初始化失败");
    
    vector<string> keys = {all_images_key, imagerank};
    if(checkRedisHealth(keys)) {
        debug("Redis数据已从持久化恢复");
        loadToPathMapOnly();
    } else {
        debug("执行全量数据加载");
        preloadImages();
    }

    // 辅助函数：简化std::function的显式构造（参数改为值传递）
    auto make_func = [](auto&& lambda) {
        return std::function<string(
            unordered_map<string, string>,  // 移除const和&
            unordered_map<string, string>   // 移除const和&
        )>(std::forward<decltype(lambda)>(lambda));
    };

    // 注册函数的Lambda参数也移除const和&
    manager_.rgister_function("/api/getRandomImage", 
        make_func([](auto header, auto post) {  // 无const和&
            return API::getRandomImage(header, post);
        })
    );

    manager_.rgister_function("/api/getimagePrefer", 
        make_func([](auto header, auto post) {  // 无const和&
            return API::getimagePrefer(header, post);
        })
    );

    manager_.rgister_function("/api/incrybyImagePrefer", 
        make_func([](auto header, auto post) {  // 无const和&
            return API::incrybyImagePrefer(header, post);
        })
    );

    manager_.rgister_function("/api/getImageRanking", 
        make_func([](auto header, auto post) {  // 无const和&
            return API::getImageRanking(header, post);
        })
    );

    manager_.rgister_function("/api/getUserFavoriteImages", 
        make_func([](auto header, auto post) {  // 无const和&
            return API::getUserFavoriteImages(header, post);
        })
    );

    debug("API初始化完成");
}

bool API::checkRedisHealth(const vector<string>& keys)
{
    try {
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
            debug("检查{}成功",key);
        }
        return true;
    } catch(const std::exception& e) {
        error("Redis健康检查失败: {}", e.what());
        return false;
    }
}

void API::loadToPathMapOnly()
{
    try {
        int count=stoi(*redis_->get("images_count"));
        path_map_.clear();
        // 对path_map_的操作加锁（若多线程访问）
        std::lock_guard<std::mutex> lock(path_map_mutex_);
        for(int i=1;i<=count;i++)
        {
            path_map_[i]=*redis_->hget(to_string(i),"image_path");
        }
        debug("从持久化Redis加载 {} 张图片到path_map_", count);
        debug("从持久化Redis恢复 {} 张图片", count);
    } catch(const exception& e) {
        error("从Redis加载path_map_失败: {}", e.what());
        preloadImages();
    }
}

void API::preloadImages()
{
    debug("加载图片数据到redis");

    auto mysql=get_one_connection();
    if(!mysql)debug("preloadImages连接mysql失败");

    char sql[1024]={0};
    snprintf(sql,sizeof(sql),"select id,image_name,prefer from images");
    if(mysql->query(sql))
    {
        int count=0;
        std::lock_guard<std::mutex> lock(path_map_mutex_); // 保护path_map_
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

        return;
    }
    debug("预加载失败");
}

shared_ptr<MysqlConn>  API::get_one_connection()
{
    ConnectionPool* pool = ConnectionPool::getConnectionPool();
    if (!pool)
    {
        error("无法获取数据库连接池");
    
        return nullptr;
    }

    auto mysql=pool->getConnection();
    if (!mysql)
    {
        error("无法获取数据库连接");
        
        return nullptr;
    }
    return mysql;
}

// 移除参数的const和&，改为值传递
string API::getRandomImage(unordered_map<string, string> header, unordered_map<string, string> post)
{
    debug("进入了post_的函数筛选getRandomImage");
    
    auto it = post.find("num");
    if(it==post.end())return "0";
    string num=it->second;
    debug("传入的num= {}",num);
    return API::getInstance()->getRandomImageFromRedis(num);
}

string API::getRandomImageFromRedis(const string& num)
{   
    debug("进入了getRandomImageFromRedis");
    
    int id = stoi(num);
    if(redis_){
        auto res = redis_->hget(num, "image_path");
        if(res)
        {
            debug("从Redis获取图片ID{}: {}", num, *res);
            std::lock_guard<std::mutex> lock(path_map_mutex_); // 保护path_map_
            path_map_[id] = *res;
            return *res;
        }
        string path="resources/Json/image"+num+".json";
        ofstream ofs(path, ios::out);

        Json::Value root;
        root["url"]=*res;
        root["id"]=num;

        if(!ofs.is_open())
        {   
            debug("resources/Json/image{}json未打开成功",num);
            return "0";
        }
        Json::StyledStreamWriter writer;
        writer.write(ofs,root);
        ofs.close();
    return  "/Json/image"+num+".json"; 
    }
    
    debug("缓存未命中,从数据库查询ID{}", num);
    return getRandomImageFromDB(num);
}

string API::getRandomImageFromDB(const string& num)
{
    debug("进入了getRandomImageFromDB");
    
    auto mysql=get_one_connection();
    if(!mysql)return "0";
    
    char sql[1024]={0};
    snprintf(sql,sizeof(sql),"select image_name from images where id=%s",num.c_str());
    if(mysql->query(sql) && mysql->next())
    {
        string image_path=mysql->value(0);
        debug("拿到的图片是{}",image_path);
       

        redis_->hset(all_images_key,num,"/images/"+image_path);
        return "/images/"+image_path;
    }
    return num;
}

// 移除call函数参数的const和&，改为值传递
any API::call(const string& name, unordered_map<string, string> header, unordered_map<string, string> post)
{
    return manager_.call_function(name,header,post);
}

// 移除参数的const和&，改为值传递
string API::getimagePrefer(unordered_map<string, string> header, unordered_map<string, string> post)
{
    debug("进入了getimagePrefer");
  
    
    auto it = post.find("id");
    if(it==post.end()){
        debug("找不到那个id");
        return "0";
    }
    string imageid=it->second;
    
    debug("要查找的图片的id是{}",imageid);
    string userid=get_userid(header,post);
    return API::getInstance()->getimagePreferFromRedis(userid,imageid);
}

string  API::getimagePreferFromRedis(const string& userid,const string& imageid)
{
    string prefer=*redis_->hget(imageid,"prefer");
    string path="resources/Json/prefer"+userid+".json";
    ofstream ofs(path, ios::out);

    Json::Value root;
    root["prefer"]=prefer;

    if(!ofs.is_open())
    {   
        debug("resources/Json/prefer{}json未打开成功",userid);
        return "0";
    }
    Json::StyledStreamWriter writer;
    writer.write(ofs,root);
    ofs.close();
    return  "/Json/prefer"+userid+".json"; 
}

// 移除get_userid参数的const和&，改为值传递
string API::get_userid(unordered_map<string, string> header, unordered_map<string, string> post)
{
    string userid;
    string tokens;
    
    auto it = header.find("Cookie");
    if(it==header.end()){
        debug("找不到那个id");
        return "0";
    }
    tokens=it->second;
    debug("tokens=={}",tokens);

    int ret=token::getInstance()->check_token(tokens,userid);
    if(!ret)
    {
        debug("解析token登陆对象失败");
    }
    return userid;
}

// 移除参数的const和&，改为值传递
string API::incrybyImagePrefer(unordered_map<string, string> header, unordered_map<string, string> post)
{
    debug("进入了incrybyImagePrefer");
    
    auto it = post.find("id");
    if(it==post.end()){
        debug("找不到那个id");
        return "0";
    }
    string imageid=it->second;
    
    debug("要查找的图片的id是{}",imageid);
    string userid=get_userid(header,post);
    return API::getInstance()->incryRedisBy(userid,imageid,"prefer",1);
}

bool API::update_image_prefer_at_mysql(const string& userid,const string& id)
{
    auto mysql=get_one_connection();
    if(!mysql)
    {
        debug("mysql获取失败");
        return false;
    }
    char sql[1024]={0};
    snprintf(sql,sizeof(sql),"update images set prefer=prefer+1 where id=%s",id.c_str());
    if(mysql->update(sql))
    {
        debug("更新总点赞成功");
    }

    snprintf(sql,sizeof(sql),"INSERT INTO user_image_likes (user_id, image_id, like_count)\
            VALUES (%s, %s, 1)\
            ON DUPLICATE KEY UPDATE\
            like_count = like_count + 1,\
            update_time = CURRENT_TIMESTAMP",userid.c_str(),id.c_str());

    if(mysql->update(sql))
    {
        debug("更新单用户点赞成功");
        return true;
    }
    return false;
}

string API::incryRedisBy(const string& userid,const string& imageid,const string& filed,const int& depth)
{
    if(update_image_prefer_at_mysql(userid,imageid)){
        debug("更新点赞数据库成功");
    }
    string count;
    try{
        auto after_prefer=redis_->hincrby(imageid,"prefer",depth);
        count=to_string(after_prefer);
        auto after_prefer_user=redis_->hincrby("user"+userid+":"+imageid,"prefer",depth);
    }catch (const exception& e) {
        error("点赞失败(Redis操作);{}", e.what());
        return "0";
    }

    string path="resources/Json/prefer"+userid+".json";
    ofstream ofs(path, ios::out);

    Json::Value root;
    root["success"]="true";
    root["count"]=count;
    if(!ofs.is_open())
    {   
        debug("resources/Json{}prefer.json未打开成功",userid);
        return "0";
    }
    Json::StyledStreamWriter writer;
    writer.write(ofs,root);
    ofs.close();
    redis_->zincrby(imagerank,1.0,imageid);
    redis_->zincrby("user"+userid+":"+imagerank,1.0,imageid);
    return  "/Json/prefer"+userid+".json"; 
}

// 移除参数的const和&，改为值传递
string API::getImageRanking(unordered_map<string, string> header, unordered_map<string, string> post)
{
    debug("进入了getImageRanking");
    string userid=get_userid(header,post);
    return API::getInstance()->getImageRankingFromRedis(userid);
}

string API::getImageRankingFromRedis(const string& userid)
{
    vector<pair<string,double>>rank;
    redis_->zrevrange(imagerank,0ll,3ll,back_inserter(rank));
    
    Json::Value root(Json::arrayValue);
    for (const auto& item : rank)
    {
        Json::Value obj;
        obj["id"] = item.first;
        obj["url"]=*redis_->hget(item.first,"image_path");
        obj["count"] = to_string(item.second);
        root.append(obj);
        debug("{}",obj["url"].asString());
    }
    StyledStreamWriter writer;
    string path="resources/Json/rank"+userid+".json";
    ofstream ofs(path, ios::out);
    if(!ofs.is_open())
    {   
        debug("resources/Json/rank{}json未打开成功",userid);
        return "0";
    }
    writer.write(ofs,root);
    ofs.close();
    return "/Json/rank"+userid+".json"; 
}

// 移除参数的const和&，改为值传递
string API::getUserFavoriteImages(unordered_map<string, string> header, unordered_map<string, string> post)
{
    debug("进入了getImageRanking");
    string userid=get_userid(header,post);
    return API::getInstance()->getUserFavoriteImages_fromRedis(userid);
}

string API::getUserFavoriteImages_fromRedis(const string& userid)
{
    vector<pair<string,double>>rank;
    redis_->zrevrange("user"+userid+":"+imagerank,0ll,3ll,back_inserter(rank));
    
    Json::Value root(Json::arrayValue);
    for (const auto& item : rank)
    {
        Json::Value obj;
        obj["id"] = item.first;
        obj["url"]=*redis_->hget(item.first,"image_path");
        obj["count"] = to_string(item.second);
        root.append(obj);
        debug("{}",obj["url"].asString());
    }
    StyledStreamWriter writer;
    string path="resources/Json/rank"+userid+".json";
    ofstream ofs(path, ios::out);
    if(!ofs.is_open())
    {   
        debug("resources/Json/rank{}json未打开成功",userid);
        return "0";
    }
    writer.write(ofs,root);
    ofs.close();
    return "/Json/rank"+userid+".json"; 
}
    