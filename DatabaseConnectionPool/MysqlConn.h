#pragma once
#include <mysql/mysql.h>
#include <iostream>
#include <chrono>
using namespace std;
using namespace chrono;

class MysqlConn
{
public:
    // 初始化数据库连接
    MysqlConn();
    // 释放数据库连接
    ~MysqlConn();
    // 连接数据库
    bool connect(string user, string password, string dbname, string ip, unsigned short port = 3306);
    // 更新数据库：insert, update, delete
    bool update(string sql);
    // 查询数据库
    bool query(string sql);
    // 遍历查询得到的结果集
    bool next();
    // 获取结果集中的字段值
    string value(int index);
    // 开启事务
    bool transaction();
    // 提交事务
    bool commit();
    // 事务回滚
    bool rollback();

    // 刷新连接时间
    void refreshTime();
    // 获取连接存活时间
    long long getAliveTime();

private:
    void freeResult();
    MYSQL* m_conn = nullptr; // 数据库连接句柄
    MYSQL_RES* m_result = nullptr;
    MYSQL_ROW m_row = nullptr;

    steady_clock::time_point m_alivetime;
};