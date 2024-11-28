#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

#define DEFAULT_QUEUE_SIZE 512

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ThreadSafeQueue(const size_t size) : size_(size) {
        if(size_ == 0)
            size_ = DEFAULT_QUEUE_SIZE;
        if(size_ > USHRT_MAX)
            size_ = USHRT_MAX;
        queue_.reserve(size_);
    }
    // Add data to the queue
    void push(const T& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(size_ > 0 && queue_.size() >= size_)
            queue_.pop();
        queue_.push(data);
        cond_var_.notify_one(); // Notify one waiting thread
    }

    // Retrieve data from the queue (blocks if queue is empty)
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_var_.wait(lock, [this]() { return !queue_.empty(); });
        if (!queue_.empty()) {
            T data = std::move(queue_.front());
            queue_.pop();
            return data;
        }
        return std::nullopt;
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_var_;
    size_t size_ = DEFAULT_QUEUE_SIZE;
};