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

Copyright (c) 2026 A-McD Technology LLC

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
#include <utility>

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
    }
    // Add data to the queue
    void push(const T& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_)
            return;
        if(size_ > 0 && queue_.size() >= size_)
            queue_.pop();
        queue_.push(data);
        // cond_var_.notify_one(); // Notify one waiting thread
        cond_var_.notify_all();
    }

    void push(T&& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_)
            return;
        if(size_ > 0 && queue_.size() >= size_)
            queue_.pop();
        queue_.push(std::move(data));
        cond_var_.notify_all();
    }

    // Retrieve data from the queue. Returns std::nullopt after the queue is closed
    // and drained so blocked consumers can exit cleanly on shutdown.
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_var_.wait(lock, [this]() { return closed_ || !queue_.empty(); });
        if (!queue_.empty()) {
            T data = std::move(queue_.front());
            queue_.pop();
            return data;
        }
        return std::nullopt;
    }

    size_t size() {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }

    void close(bool clearPending = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        if (clearPending) {
            queue_ = std::queue<T>();
        }
        cond_var_.notify_all();
    }

    void reopen(bool clearPending = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = false;
        if (clearPending) {
            queue_ = std::queue<T>();
        }
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_var_;
    size_t size_ = DEFAULT_QUEUE_SIZE;
    bool closed_ = false;
};
