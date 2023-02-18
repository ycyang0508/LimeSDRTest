#include <mutex>
#include <condition_variable>
#include <queue>

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() {};
    ~ThreadSafeQueue() {};


    void push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(item);
        lock.unlock();
        cond_.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return !queue_.empty(); });
        T item = queue_.front();
        queue_.pop();
        return item;
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

