#include <stack>
#include <memory>
#include <mutex>
#include <condition_variable>
//下面是测试需要的
#include <iostream>
#include <cassert>
#include <vector>
#include <thread>
#include <atomic>
#include <future>

template<typename T>
class SafeStack{
    public:
        SafeStack() = default;

        SafeStack(const SafeStack& other){
            std::unique_lock<std::mutex> lock_this(mtx_, std::defer_lock);
            std::unique_lock<std::mutex> lock_other(other.mtx_, std::defer_lock);
            std::lock(lock_this, lock_other); // 避免死锁   
            stack_ = other.stack_;
        }
        SafeStack& operator=(const SafeStack&) = delete;

        void push(T new_value){
            std::unique_lock<std::mutex> lock(mtx_);
            stack_.emplace(std::make_shared<T>(std::move(new_value)));
            cv_.notify_one();
        }

        std::shared_ptr<T> pop(){
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait
            (lock,
            [this](){
                if(stack_.empty()){
                    return false;
                }else{
                    return true;
                }
            }
            );
            std::shared_ptr<T> res = stack_.top();
            stack_.pop();
            return res;
        }

        void pop(T& value){
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait
            (lock,
            [this](){
                if(stack_.empty()){
                    return false;
                }else{
                    return true;
                }
            }
            );
            value = std::move(*stack_.top());
            stack_.pop();
        }

        bool empty(){
            std::unique_lock<std::mutex> lock(mtx_);
            return stack_.empty();
        }

        bool try_pop(T& value){
            std::unique_lock<std::mutex> lock(mtx_);
            if(stack_.empty()){
                return false;
            }
            value = std::move(*stack_.top());
            stack_.pop();
            return true;
        }

        std::shared_ptr<T> try_pop(){
            std::unique_lock<std::mutex> lock(mtx_);
            if(stack_.empty()){
                return std::shared_ptr<T>();
            }
            std::shared_ptr<T> res = stack_.top();
            stack_.pop();
            return res;
        }    
    private:
        std::stack<std::shared_ptr<T>> stack_;
        std::mutex mtx_;
        std::condition_variable cv_;
};

//单线程测试
void test_single_thread() {
    SafeStack<int> stack;

    // 测试空栈判断
    assert(stack.empty());

    // 测试 push 和 pop
    stack.push(1);
    stack.push(2);
    stack.push(3);
    assert(!stack.empty());

    // 测试 try_pop
    int val = 0;
    bool success = stack.try_pop(val);
    assert(success && val == 3);

    // 测试普通 pop
    auto ptr = stack.pop();
    assert(ptr != nullptr && *ptr == 2);

    // 测试引用版本 pop
    stack.pop(val);
    assert(val == 1);

    // 测试空栈行为
    assert(stack.empty());
    assert(!stack.try_pop(val));
    assert(stack.try_pop() == nullptr);

    std::cout << "[PASS] Single-thread test passed!\n";
}
//多线程测试
void test_concurrent() {
    SafeStack<int> stack;
    const int kThreadCount = 4;
    const int kPushPerThread = 10000;
    std::atomic<int> push_count{0};
    std::atomic<int> pop_count{0};
    std::vector<std::future<void>> futures;

    // 生产者线程函数
    auto producer = [&]() {
        for (int i = 0; i < kPushPerThread; ++i) {
            stack.push(++push_count);
        }
    };

    // 消费者线程函数
    auto consumer = [&]() {
        int local_pop = 0;
        while (local_pop < kPushPerThread) {
            if (auto ptr = stack.try_pop()) {
                ++local_pop;
                ++pop_count;
            }
        }
    };

    // 创建 2 个生产者，2 个消费者
    for (int i = 0; i < 2; ++i) {
        futures.emplace_back(std::async(std::launch::async, producer));
        futures.emplace_back(std::async(std::launch::async, consumer));
    }

    // 等待所有线程完成
    for (auto& f : futures) f.wait();

    // 验证总数
    assert(push_count == 2 * kPushPerThread);
    assert(pop_count == 2 * kPushPerThread);

    std::cout << "[PASS] Concurrent test passed!\n";
}
//阻塞测试
void test_blocking_pop() {
    SafeStack<int> stack;
    std::atomic<bool> start_consumer{false};
    std::atomic<bool> item_popped{false};

    // 消费者线程
    auto consumer = std::async(std::launch::async, [&]() {
        start_consumer = true;
        int val = 0;
        stack.pop(val);  // 应该阻塞直到有数据
        item_popped = true;
        assert(val == 42);
    });

    // 确保消费者已经启动并进入等待
    while (!start_consumer) std::this_thread::yield();

    // 验证消费者尚未弹出数据
    assert(!item_popped);

    // 生产者推送数据
    stack.push(42);

    // 等待消费者完成
    consumer.wait();
    assert(item_popped);

    std::cout << "[PASS] Blocking pop test passed!\n";
}

int main() {
    test_single_thread();
    test_concurrent();
    test_blocking_pop();
    return 0;
}