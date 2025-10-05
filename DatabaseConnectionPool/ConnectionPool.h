#pragma once
#include<fstream>
#include<thread>
#include<queue>
#include<mutex>
#include<condition_variable>
#include"MysqlConn.h"
#include<jsoncpp/json/json.h>
using namespace std;
using namespace Json;
class ConnectionPool
{
private:
	ConnectionPool();
	~ConnectionPool();
	bool parseJsonFile();
	void produceConnection();
	void removeConnection();
	void addConnection();
	queue<MysqlConn*>m_connections;
	string m_ip;
	string m_user;
	string m_passwd;
	string m_dbName;
	unsigned short m_port;
	int m_minSize;
	int m_maxSize;
	int m_timeout; 
	int m_maxidletime;
	mutex m_mtx;
	condition_variable m_conditon;
public:
	static ConnectionPool* getConnectionPool();
	ConnectionPool(const ConnectionPool& others) = delete;
	ConnectionPool& operator=(const ConnectionPool& others) = delete;
	
	shared_ptr<MysqlConn> getConnection();
};