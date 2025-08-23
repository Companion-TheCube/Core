/*
██╗   ██╗████████╗██╗██╗     ███████╗   ██╗  ██╗
██║   ██║╚══██╔══╝██║██║     ██╔════╝   ██║  ██║
██║   ██║   ██║   ██║██║     ███████╗   ███████║
██║   ██║   ██║   ██║██║     ╚════██║   ██╔══██║
╚██████╔╝   ██║   ██║███████╗███████║██╗██║  ██║
 ╚═════╝    ╚═╝   ╚═╝╚══════╝╚══════╝╚═╝╚═╝  ╚═╝
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
#ifndef UTILS_H
#define UTILS_H
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <latch>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <cppcodec/base64_rfc4648.hpp>

#ifndef _TASK_QUEUE_H_
#define _TASK_QUEUE_H_
template <typename T>
class TaskQueue {
public:
    TaskQueue() {};
    void push(T task)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tasks_.push_back(std::move(task));
        }
        condition_.notify_one();
    }
    T pop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !tasks_.empty(); });
        auto task = std::move(tasks_.back());
        tasks_.pop_back();
        return task;
    }
    T shift()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !tasks_.empty(); });
        auto task = std::move(tasks_.front());
        tasks_.erase(tasks_.begin());
        return task;
    }
    T peek()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.front();
    }
    T peek(int index)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.at(index);
    }
    size_t size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.size();
    }

private:
    std::vector<T> tasks_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

template <typename T, typename D>
class TaskQueueWithData {
public:
    TaskQueueWithData() {};
    void push(T task, D data)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tasks_.push_back(std::move(task));
            data_.push_back(std::move(data));
        }
        condition_.notify_one();
    }
    T pop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !tasks_.empty(); });
        auto task = std::move(tasks_.back());
        tasks_.pop_back();
        return task;
    }
    T shift()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !tasks_.empty(); });
        auto task = std::move(tasks_.front());
        tasks_.erase(tasks_.begin());
        return task;
    }
    T peek()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.front();
    }
    T peek(int index)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.at(index);
    }
    size_t size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.size();
    }
    D dataPop()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto data = std::move(data_.back());
        data_.pop_back();
        return data;
    }

private:
    std::vector<T> tasks_;
    std::vector<D> data_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

#endif

#define INIT_COUNTDOWN_TOTAL 1

#ifndef _UTILS_H_
#define _UTILS_H_

void genericSleep(int ms);
void monitorMemoryAndCPU();
std::string getMemoryFootprint();
std::string getCpuUsage();

// ------------------------------------------------------------
// Global configuration loader (reads .env once, accessible app-wide)
// ------------------------------------------------------------
namespace Config {
// Load key/value pairs from a .env file (KEY=VALUE per line). Environment
// variables override file values. Safe to call multiple times; subsequent calls
// merge/override.
void loadFromDotEnv(const std::string& path = ".env");

// Get a config value by key. If not loaded yet, performs a one-time lazy load
// from ".env" automatically. Returns defaultValue if key is not present.
std::string get(const std::string& key, const std::string& defaultValue = "");

// Insert or override a key at runtime (useful for tests or dynamic config).
void set(const std::string& key, const std::string& value);

// Whether a key exists in the configuration map (after lazy load).
bool has(const std::string& key);
} // namespace Config

#endif

#ifndef _COUNTDOWN_LATCH_H_
#define _COUNTDOWN_LATCH_H_
#include <condition_variable>
#include <mutex>

class CountingLatch {
public:
    explicit CountingLatch(int count)
        : count_(count)
    {
    }

    void count_up(int n = 1)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        count_ += n;
    }

    void count_down()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (--count_ == 0) {
            cv_.notify_all();
        }
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return count_ == 0; });
    }

private:
    int count_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

std::string sha256(std::string input);
std::string crc32(std::string input);

std::vector<unsigned char> base64_decode_cube(std::string const& encoded_string);
std::string base64_encode_cube(const std::vector<unsigned char>& bytes_to_encode);
std::string base64_encode_cube(const std::string& bytes_to_encode);

#endif
#endif // UTILS_H
