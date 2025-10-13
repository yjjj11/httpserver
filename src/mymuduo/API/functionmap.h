#include<any>
#include<iostream>
#include<unordered_map>
#include<memory>
#include<functional>
#include<tuple>
#include<utility>
#include<typeinfo>  
#include"../log.h"
using namespace std;

class FunctionWrapper{
public:
    virtual ~FunctionWrapper() = default;
    virtual any call(const any& packed_args) = 0;
};

template<typename Ret, typename... Args>
class ConcreteFunction : public FunctionWrapper {
public:
    using FuncType = function<Ret(Args...)>;

    explicit ConcreteFunction(FuncType func) : func_(move(func)) {}

    any call(const any& packed_args) override {
        try {
            // 处理无参数情况
            if constexpr (sizeof...(Args) == 0) {
                if constexpr (is_void_v<Ret>) { 
                    func_(); 
                    return {}; 
                } else { 
                    return func_(); 
                }
            } else {
                // 关键修复：确保any_cast的类型与实际打包类型完全一致
                auto args = any_cast<tuple<Args...>>(packed_args);
                
                // 调用函数并处理返回值
                if constexpr (is_void_v<Ret>) { 
                    apply(func_, args); 
                    return {}; 
                } else { 
                    return apply(func_, args); 
                }
            }
        } catch (const bad_any_cast& e) {
            // 增强错误信息：打印期望类型和实际类型
            string expected = typeid(tuple<Args...>).name();
            string actual = packed_args.type().name();
            error("函数参数类型转换失败: {}", e.what());
            error("期望类型: {}", expected);
            error("实际类型: {}", actual);
            throw; // 继续抛出异常让上层处理
        } catch (const exception& e) {
            error("函数调用异常: {}", e.what());
            throw;
        }
    }

private:
    FuncType func_;
};

class FunctionManager{
public:

    template<typename Ret, typename... Args>
    void rgister_function(const string& name, Ret (*func)(Args...)) {
        funcs_[name] = make_unique<ConcreteFunction<Ret, Args...>>(func);
    }

    // 注册std::function（支持Lambda和其他可调用对象）
    template<typename Ret, typename... Args>
    void rgister_function(const string& name, function<Ret(Args...)> func) {
        funcs_[name] = make_unique<ConcreteFunction<Ret, Args...>>(move(func));
    }

    // 调用函数 - 关键修复：完美转发保持参数类型信息
    template<typename... Args>
    any call_function(const string& name, Args&&... args) {
        auto it = funcs_.find(name);
        if (it == funcs_.end()) {
            debug("函数{}不存在", name);
            return any{};
        }

        try {
            // 完美转发参数，保留const和引用属性
            auto packed = make_tuple(forward<Args>(args)...);
            return it->second->call(packed);
        } catch (const exception& e) {
            error("调用函数{}失败: {}", name, e.what());
            return any{};
        }
    }

private:
    unordered_map<string, unique_ptr<FunctionWrapper>> funcs_;
};
