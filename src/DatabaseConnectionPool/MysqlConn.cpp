#include"MysqlConn.h"

MysqlConn::MysqlConn()
{
    m_conn = mysql_init(nullptr);
    mysql_set_character_set(m_conn, "utf8");  // 设置字符集为utf8
}

MysqlConn::~MysqlConn()
{
    if (m_conn != nullptr)
        mysql_close(m_conn);  // 关闭数据库连接
    freeResult();  // 释放查询结果
}

// 连接数据库
bool MysqlConn::connect(string user, string password, string dbname, string ip, unsigned short port)
{
    MYSQL* ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), 
                                   password.c_str(), dbname.c_str(), port, NULL, 0);
    return ptr != nullptr;  // 连接成功返回true
}

// 执行更新操作（insert、update、delete）
bool MysqlConn::update(string sql)
{
    int ret = mysql_query(m_conn, sql.c_str());
    if (ret) return false;  // 执行失败返回false
    return true;
}

// 执行查询操作（select）
bool MysqlConn::query(string sql)
{
    freeResult();  // 释放之前的查询结果
    int ret = mysql_query(m_conn, sql.c_str());
    if (ret) 
    {
        cout<<"执行查询操作失败\n";
        return false;  // 执行失败返回false
    }
    m_result = mysql_store_result(m_conn);  // 存储查询结果
    return true;
}

// 遍历查询结果集
bool MysqlConn::next()
{
    if (m_result != nullptr)
    {
        m_row = mysql_fetch_row(m_result);  // 获取下一行数据
        if (m_row != nullptr) return true;  // 存在数据返回true
    }
    return false;
}

// 获取结果集中指定索引的字段值
string MysqlConn::value(int index)
{
    int columnCount = mysql_num_fields(m_result);  // 获取字段数量
    if (index >= columnCount || index < 0) return string();  // 索引越界返回空
    
    char* val = m_row[index];
    if (val == nullptr) return string();  // 字段值为空返回空
    
    // 获取字段值的实际长度（避免包含'\0'导致截断）
    size_t length = mysql_fetch_lengths(m_result)[index];
    return string(val, length);
}

// 开启事务（关闭自动提交）
bool MysqlConn::transaction()
{
    return mysql_autocommit(m_conn, false);
}

// 提交事务
bool MysqlConn::commit()
{
    return mysql_commit(m_conn);
}

// 回滚事务
bool MysqlConn::rollback()
{
    return mysql_rollback(m_conn);
}

// 释放查询结果集
void MysqlConn::freeResult()
{
    if (m_result == nullptr) return;
    mysql_free_result(m_result);
    m_result = nullptr;
}

// 刷新连接时间（更新最后活跃时间）
void MysqlConn::refreshTime()
{
    m_alivetime = steady_clock::now();
}

// 获取连接存活时间（毫秒）
long long MysqlConn::getAliveTime()
{
    nanoseconds res = steady_clock::now() - m_alivetime;
    milliseconds millisec = duration_cast<milliseconds>(res);
    return millisec.count();
}