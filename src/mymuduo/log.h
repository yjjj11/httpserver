#pragma once
#include"Timestamp.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <iostream>
#include <condition_variable>
#include <queue>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <format>
using namespace std;

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

template<typename T>
class blockQueue
{
public:
    blockQueue() = default;
    ~blockQueue() = default;

    void push(const T& t)
    {
        //cout << "[blockQueue] 准备push日志\n";  // 新增
        lock_guard<mutex> locker(mtx_);
        if (nonBlock_) {  // 新增：判断队列是否已进入非阻塞状态（析构中）
            cout << "[blockQueue] 队列已非阻塞，push失败\n";
            return;
        }
        queue_.push(t);
        notEmpty_.notify_one();
        //cout << "[blockQueue] push日志成功\n";  // 新增
    }

    void push(T&& t)
    {
        lock_guard<mutex> locker(mtx_);
        queue_.push(std::move(t));
        notEmpty_.notify_one();
    }

    bool pop(T& t)
    {
        unique_lock<mutex> lock(mtx_);
        notEmpty_.wait(lock, [this]() {
            return !queue_.empty() || nonBlock_;
            });

        if (queue_.empty()) return false;
        t = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void setNonBlock() {
        lock_guard<mutex> locker(mtx_);
        nonBlock_ = true;
        notEmpty_.notify_all();
    }

    bool empty() {
        lock_guard<mutex> locker(mtx_);
        return queue_.empty();
    }

private:
    bool nonBlock_ = false;
    queue<T> queue_;
    condition_variable notEmpty_;
    mutex mtx_;
};

class Logger
{
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    void setLogLevel(LogLevel level) {
        lock_guard<mutex> locker(levelMtx_);
        filterLevel_ = level;
    }

    void writeLog(LogLevel level, const string& content) {
        {
            lock_guard<mutex> locker(levelMtx_);
            if (level < filterLevel_) {
                return;
            }
        }

        string logMsg = getTimeStamp() + " " + getLevelStr(level) + " " + content;
        queue_.push(std::move(logMsg));
    }

   ~Logger() {
        cout << "[Logger] 开始析构，设置 nonBlock=true\n";  // 新增
        queue_.setNonBlock();
        if (writeThread_ && writeThread_->joinable()) {
            cout << "[Logger] 等待写线程退出\n";  // 新增
            writeThread_->join();
            delete writeThread_;
            writeThread_ = nullptr;
            cout << "[Logger] 写线程已退出\n";  // 新增
        }
        cout << "[Logger] 析构完成\n";  // 新增
    }

private:
    Logger()
        : filterLevel_(LogLevel::DEBUG)
    {
        writeThread_ = new thread(&Logger::work, this);
    }

    void work() {
        while (true) {
            string logMsg;
            if (!queue_.pop(logMsg)) {
                break;
            }

            lock_guard<mutex> locker(fileMtx_);
            ofstream ofs(logPath_, ios::app);
            if (ofs.is_open()) {
                ofs << logMsg << endl;
                ofs.close();
            }
            else {
                //cerr << "[Logger Error] Failed to open log file: " << logPath_ << endl;
            }
        }
    }

    string getTimeStamp() {
        Timestamp ts = Timestamp::now();
        return "[" + ts.tostring() + "]"; 
    }

    string getLevelStr(LogLevel level) {
        switch (level) {
        case LogLevel::DEBUG:   return "[DEBUG]";
        case LogLevel::INFO:    return "[INFO]";
        case LogLevel::WARNING: return "[WARNING]";
        case LogLevel::ERROR:   return "[ERROR]";
        case LogLevel::FATAL:   return "[FATAL]";
        default:                return "[UNKNOWN]";
        }
    }

private:
    blockQueue<string> queue_;
    thread* writeThread_ = nullptr;
    mutex fileMtx_;
    const string logPath_ = "app.log";
    LogLevel filterLevel_;
    mutex levelMtx_;
};

#define debug(fmt, ...)  Logger::getInstance().writeLog(LogLevel::DEBUG,   std::format(fmt, ##__VA_ARGS__))
#define Info(fmt, ...)   Logger::getInstance().writeLog(LogLevel::INFO,    std::format(fmt, ##__VA_ARGS__))
#define warning(fmt, ...)Logger::getInstance().writeLog(LogLevel::WARNING, std::format(fmt, ##__VA_ARGS__))
#define error(fmt, ...)  Logger::getInstance().writeLog(LogLevel::ERROR,   std::format(fmt, ##__VA_ARGS__))
#define fatal(fmt, ...)  Logger::getInstance().writeLog(LogLevel::FATAL,   std::format(fmt, ##__VA_ARGS__))