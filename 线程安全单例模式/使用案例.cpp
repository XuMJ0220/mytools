#include "singleton.h"
#include <thread>
#include <iostream>
//下面是使用
//把需要单例的类拿去继承Singleton
                          //这里Singleton里面的类型填写需要单例的类 
class Test : public xumj::Singleton<Test>{

    friend class xumj::Singleton<Test>;//这里必须添加有元，因为需要让xumj::Singleton<Test>去访问Test的私有属性

    public:
        void hello(){}
        ~Test(){}
    private:
        Test(){}
};
/****************************************************************************************************/
//下面是测试
int main(){
    
    std::thread t1(
        []()
        {
           auto ptr = Test::GetInstance();
           std::cout<<ptr.get()<<std::endl; 
        }
    );
    std::thread t2(
        []()
        {
            auto ptr = Test::GetInstance();
            std::cout<<ptr.get()<<std::endl; 
        }
    );
    t1.join();
    t2.join();
    return 0;
}