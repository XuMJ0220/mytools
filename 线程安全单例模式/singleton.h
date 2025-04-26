#ifndef XUMJ_SINGLETON_H_
#define XUMJ_SINGLETON_H_

#include <memory>
#include <mutex>

namespace xumj{

    template<typename T>
    class Singleton{
        //protected里面的会变成基类的private
        protected:
            Singleton(){}
            Singleton(const Singleton<T>&) = delete;
            Singleton<T>& operator=(const Singleton<T>&) = delete;
            //用了智能指针，因为根本就不知道是哪个线程初始化的所以不知道去哪个线程delete，用了智能指针可以避免这个问题
            static std::shared_ptr<T> instance_;
        public:
            static std::shared_ptr<T> GetInstance(){
                static std::once_flag once_flag_;
                //使用call_once，保证不同线程之间只能执行一次下面这个lambda
                std::call_once(once_flag_,
                    [&]()
                    {
                        //外面的T的构造函数将是私有的，就不能用make_shared了
                        //instance_ = std::make_shared<T>();
                        //需要用下面的方法
                        instance_ = std::shared_ptr<T>(new T);
                    }
                );
                return instance_;
            }
    };
    template<typename T>
    std::shared_ptr<T> Singleton<T>::instance_ = nullptr;
}

#endif // !XUMJ_SINGLETON_H_