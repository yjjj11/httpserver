#include<any>
#include<iostream>
#include<unordered_map>
#include<memory>
#include"../log.h"
using namespace std;

class FunctionWrapper{
public:
    virtual ~FunctionWrapper() = default;
    virtual any call() = 0;//强制实现
};

template<typename Ret>
class ConcreteFunction : public FunctionWrapper
{
public:
    explicit ConcreteFunction(Ret (*func)()) :func_(func){}

    any call() override{return func_();}
private:
    Ret (*func_)();
};

class FunctionManager{
public:
    template<typename Ret>
    void rgister_function(const string& name,Ret (*func)())
    {
        funcs_[name]=make_unique<ConcreteFunction<Ret>>(func);
    }

    any call_function(const string& name)
    {
        auto it= funcs_.find(name);
        if(it==funcs_.end()){
            debug("函数{}不存在",name);
        }
        return it->second->call();
    }
private:
    unordered_map<string,unique_ptr<FunctionWrapper>> funcs_;
};