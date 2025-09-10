#include"ThreadPoll.h"
#include<iostream>
using namespace std;
ThreadPool::ThreadPool(size_t threadnum,const string& threadtype):stop_(false),threadtype_(threadtype)
{    
    for(int i=0;i<threadnum;i++)
    {
        threads_.emplace_back([this]{
            printf("create %s thread(%ld)\n", threadtype_.c_str(), syscall(SYS_gettid)); 

            while(stop_==false)
            {
                function<void()>task;   //用于存放出对的元素
                {
                    unique_lock<mutex> lock(mutex_);

                    condition_.wait(lock,[this]{
                        return (stop_==true)||(taskqueue_.empty()==false);
                    });

                    if(this->stop_==true&&taskqueue_.empty()==true)return;
                    task=move(taskqueue_.front());
                    taskqueue_.pop();
                }
                    //printf("%s(%ld) execute task.\n", threadtype_.c_str(), syscall(SYS_gettid));
                    task();//执行任务
                
            }
        });
    }
}


void ThreadPool::addtask(function<void()> task)
{
    lock_guard<mutex> lock(mutex_);
    taskqueue_.push(task);
    condition_.notify_one();    //唤醒一个线程
}

ThreadPool::~ThreadPool()
{
    Stop();
}

size_t  ThreadPool::size()
{
    return threads_.size();
}

void ThreadPool::Stop()
{
    if(stop_)return;
    stop_=true;

    condition_.notify_all();//唤醒全部线程
    for(auto &th:threads_) th.join();//等待全部任务执行完后退出
}