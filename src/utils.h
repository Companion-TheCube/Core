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
#include "psapi.h"
#include <windows.h>
#endif
#endif

#ifndef _TASK_QUEUE_H_
#define _TASK_QUEUE_H_
class TaskQueue {
public:
    void push(std::function<void()> task)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tasks_.push_back(std::move(task));
        }
        condition_.notify_one();
    }
    std::function<void()> pop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !tasks_.empty(); });
        auto task = std::move(tasks_.back());
        tasks_.pop_back();
        return task;
    }
    std::function<void()> peek(){
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.front();
    }
    std::function<void()> peek(int index){
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.at(index);
    }
    size_t size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.size();
    }

private:
    std::vector<std::function<void()>> tasks_;
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


#endif// UTILS_H
