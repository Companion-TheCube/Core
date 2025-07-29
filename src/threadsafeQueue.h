/*
████████╗██╗  ██╗██████╗ ███████╗ █████╗ ██████╗ ███████╗ █████╗ ███████╗███████╗ ██████╗ ██╗   ██╗███████╗██╗   ██╗███████╗   ██╗  ██╗
╚══██╔══╝██║  ██║██╔══██╗██╔════╝██╔══██╗██╔══██╗██╔════╝██╔══██╗██╔════╝██╔════╝██╔═══██╗██║   ██║██╔════╝██║   ██║██╔════╝   ██║  ██║
   ██║   ███████║██████╔╝█████╗  ███████║██║  ██║███████╗███████║█████╗  █████╗  ██║   ██║██║   ██║█████╗  ██║   ██║█████╗     ███████║
   ██║   ██╔══██║██╔══██╗██╔══╝  ██╔══██║██║  ██║╚════██║██╔══██║██╔══╝  ██╔══╝  ██║▄▄ ██║██║   ██║██╔══╝  ██║   ██║██╔══╝     ██╔══██║
   ██║   ██║  ██║██║  ██║███████╗██║  ██║██████╔╝███████║██║  ██║██║     ███████╗╚██████╔╝╚██████╔╝███████╗╚██████╔╝███████╗██╗██║  ██║
   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝     ╚══════╝ ╚══▀▀═╝  ╚═════╝ ╚══════╝ ╚═════╝ ╚══════╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License

Copyright (c) 2025 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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