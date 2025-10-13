#include "ConnectionPool.h"

// 获取连接池单例
ConnectionPool* ConnectionPool::getConnectionPool()
{
    static ConnectionPool pool;
    return &pool;
}

// 解析JSON配置文件
bool ConnectionPool::parseJsonFile()
{
    ifstream ifs("dbconf.json");
    Reader rd;
    Value root;
    rd.parse(ifs, root);  // 解析到root中

    if (root.isObject())
    {
        m_ip = root["ip"].asString();
        m_port = root["port"].asInt();
        m_user = root["userName"].asString();
        m_passwd = root["password"].asString();
        m_dbName = root["dbName"].asString();
        m_minSize = root["minSize"].asInt();
        m_maxSize = root["maxSize"].asInt();
        m_maxidletime = root["maxIdleTime"].asInt();
        m_timeout = root["timeout"].asInt();
        return true;
    }
    return false;
}

// 连接池构造函数
ConnectionPool::ConnectionPool()
{
    // 解析配置文件
    if (!parseJsonFile()) return;
    
    // 初始化最小连接数
    for (int i = 0; i < m_minSize; i++)
    {
        addConnection();
    }

    // 启动生产者线程和清理线程
    thread producer(&ConnectionPool::produceConnection, this);
    thread remover(&ConnectionPool::removeConnection, this);

    producer.detach();  // 生产者线程后台运行
    remover.detach();   // 清理线程后台运行
}

// 生产连接（当连接数低于最小值时）
void ConnectionPool::produceConnection()
{
    while (true)
    {
        unique_lock<mutex> lock(m_mtx);
        // 当连接数大于等于最小连接数时等待
        while (m_connections.size() >= m_minSize)
        {
            m_conditon.wait(lock);
        }
        // 添加新连接
        addConnection();
        // 通知等待的线程
        m_conditon.notify_all();
    }
}

// 移除超时空闲连接
void ConnectionPool::removeConnection()
{
    while (true)
    {
        // 每隔1秒检查一次
        this_thread::sleep_for(chrono::seconds(1));
        unique_lock<mutex> lock(m_mtx);
        // 当连接数大于最小连接数时检查超时
        while (m_connections.size() > m_minSize)
        {
            MysqlConn* conn = m_connections.front();
            // 如果连接空闲时间超过最大限制则移除
            if (conn->getAliveTime() >= m_maxidletime)
            {
                m_connections.pop();
                delete conn;  // 释放连接
            }
            else
            {
                // 队列是有序的，前面的没超时后面的也不会超时
                break;
            }
        }
    }
}

// 添加新连接到连接池
void ConnectionPool::addConnection()
{
    MysqlConn* conn = new MysqlConn;
    // 连接数据库
    int ret = conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port);
    if (!ret)
    {
        cout << "连接失败: " << ret << endl;
    }
    // 刷新连接时间
    conn->refreshTime();
    // 添加到连接池
    m_connections.push(conn);
}

// 从连接池获取连接
shared_ptr<MysqlConn> ConnectionPool::getConnection()
{
    unique_lock<mutex> lock(m_mtx);
    // 等待连接可用，超时则继续等待
    while (m_connections.empty())
    {
        if (cv_status::timeout == m_conditon.wait_for(lock, chrono::milliseconds(m_timeout)))
        {
            if (m_connections.empty())
            {
                continue;
            }
        }
    }
    // 用智能指针管理连接，自定义删除器（归还连接到池而不是释放）
    shared_ptr<MysqlConn> connptr(m_connections.front(), [this](MysqlConn* conn) {
        lock_guard<mutex> lg(m_mtx);
        m_connections.push(conn);
        conn->refreshTime();
    });
    m_connections.pop();
    // 通知生产者线程可能需要补充连接
    m_conditon.notify_all();

    return connptr;
}

// 连接池析构函数
ConnectionPool::~ConnectionPool()
{
    // 释放所有连接
    while (!m_connections.empty())
    {
        MysqlConn* conn = m_connections.front();
        m_connections.pop();
        delete conn;
    }
}