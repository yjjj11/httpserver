#pragma once
#include <sys/syscall.h>  // 包含 syscall() 和 SYS_gettid 定义
#include <unistd.h> 
#include <vector>     // 用于 std::vector
#include <queue>      // 用于 std::queue
#include <mutex>      // 用于 std::mutex
#include <thread>     // 用于 std::thread
#include <condition_variable>  // 用于 std::condition_variable
#include <functional> // 用于 std::function
#include <future>     // 用于 std::future（如果后续任务涉及异步返回值等场景，这里先引入）
#include <atomic>     // 用于 std::atomic_bool

class ThreadPool
{
private:
    std::vector<std::thread> threads_;      // 线程池中的线程。
    std::queue<std::function<void()>> taskqueue_; // 任务队列。
    std::mutex mutex_;                       // 任务队列同步的互斥锁。
    std::condition_variable condition_;      // 任务队列同步的条件变量。
    std::atomic_bool stop_;                  // 在析构函数中，把 stop_ 的值设置为 true，全部的线程将退出。
    std::string threadtype_;
public:
    // 在构造函数中将启动 threadnum 个线程，
    ThreadPool(size_t threadnum,const std::string& threadtype);

    // 把任务添加到队列中。
    void addtask(std::function<void()> task);
    size_t  size();
    // 在析构函数中将停止线程。
    ~ThreadPool();
    void Stop();
};