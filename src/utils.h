#pragma once
#ifndef UTILS_H
#define UTILS_H
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <latch>
#include <logger.h>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#ifndef WIN32_INCLUDED
#define WIN32_INCLUDED
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#endif
#ifdef _WIN32
#include "psapi.h"
#include <codecvt>
#endif

#ifndef _TASK_QUEUE_H_
#define _TASK_QUEUE_H_
template <typename T>
class TaskQueue {
public:
    TaskQueue(){};
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
    TaskQueueWithData(){};
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


#ifdef _WIN32
std::string convertWCHARToString(const WCHAR* wstr);
void convertStringToWCHAR(const std::string& str, WCHAR* wstr);
#endif

std::string sha256(std::string input);
std::string crc32(std::string input);

#endif
#endif // UTILS_H