/*
███████╗██╗   ██╗███╗   ██╗ ██████╗████████╗██╗ ██████╗ ███╗   ██╗        ██████╗ ██╗   ██╗███╗   ██╗███╗   ██╗███████╗██████╗         ██╗███╗   ███╗██████╗ ██╗         ██████╗██████╗ ██████╗ 
██╔════╝██║   ██║████╗  ██║██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║        ██╔══██╗██║   ██║████╗  ██║████╗  ██║██╔════╝██╔══██╗        ██║████╗ ████║██╔══██╗██║        ██╔════╝██╔══██╗██╔══██╗
█████╗  ██║   ██║██╔██╗ ██║██║        ██║   ██║██║   ██║██╔██╗ ██║        ██████╔╝██║   ██║██╔██╗ ██║██╔██╗ ██║█████╗  ██████╔╝        ██║██╔████╔██║██████╔╝██║        ██║     ██████╔╝██████╔╝
██╔══╝  ██║   ██║██║╚██╗██║██║        ██║   ██║██║   ██║██║╚██╗██║        ██╔══██╗██║   ██║██║╚██╗██║██║╚██╗██║██╔══╝  ██╔══██╗        ██║██║╚██╔╝██║██╔═══╝ ██║        ██║     ██╔═══╝ ██╔═══╝ 
██║     ╚██████╔╝██║ ╚████║╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║███████╗██║  ██║╚██████╔╝██║ ╚████║██║ ╚████║███████╗██║  ██║███████╗██║██║ ╚═╝ ██║██║     ███████╗██╗╚██████╗██║     ██║     
╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝╚══════╝╚═╝╚═╝     ╚═╝╚═╝     ╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
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


#include "functionRegistry.h"

namespace DecisionEngine {

// FunctionRunner implementation moved to its own file
FunctionRunner::FunctionRunner() {}

FunctionRunner::~FunctionRunner() { stop(); }

void FunctionRunner::start(size_t numThreads) {
    std::lock_guard<std::mutex> guard(startStopMutex_);
    if (running_) return;
    if (numThreads == 0) numThreads = 1;
    numThreads_ = numThreads;
    running_ = true;
    workers_.reserve(numThreads_);
    for (size_t i = 0; i < numThreads_; ++i) {
        workers_.emplace_back([this]() { this->workerLoop(); });
    }
}

void FunctionRunner::stop() {
    std::lock_guard<std::mutex> guard(startStopMutex_);
    if (!running_) return;
    running_ = false;
    for (size_t i = 0; i < workers_.size(); ++i) {
        Task t; t.work = [](){ return nlohmann::json(); };
        queue_.push(t);
    }
    for (auto &th : workers_) if (th.joinable()) th.join();
    workers_.clear();
}

void FunctionRunner::enqueue(Task&& task) { queue_.push(std::move(task)); }

void FunctionRunner::enqueueFunctionCall(const std::string& functionName,
    std::function<nlohmann::json()> work,
    std::function<void(const nlohmann::json&)> onComplete,
    uint32_t timeoutMs) {
    Task t; t.name = functionName; t.work = std::move(work); t.onComplete = std::move(onComplete); t.timeoutMs = timeoutMs; enqueue(std::move(t));
}

void FunctionRunner::enqueueCapabilityCall(const std::string& capabilityName,
    std::function<nlohmann::json()> work,
    std::function<void(const nlohmann::json&)> onComplete,
    uint32_t timeoutMs) {
    Task t; t.name = capabilityName; t.work = std::move(work); t.onComplete = std::move(onComplete); t.timeoutMs = timeoutMs; enqueue(std::move(t));
}

void FunctionRunner::workerLoop() {
    const uint32_t baseBackoffMs = 100;
    while (running_) {
        auto opt = queue_.pop();
        if (!opt.has_value()) continue;
        Task task = std::move(opt.value());
        if (!running_) break;

        // Rate limiting
        if (task.rateLimitMs > 0) {
            TimePoint last;
            {
                std::lock_guard<std::mutex> lock(lastCalledMutex_);
                auto it = lastCalledMap_.find(task.name);
                if (it != lastCalledMap_.end()) last = it->second;
            }
            if (last != TimePoint::min()) {
                auto now = std::chrono::system_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
                if (elapsed < (int)task.rateLimitMs) {
                    uint32_t waitMs = task.rateLimitMs - static_cast<uint32_t>(elapsed);
                    if (task.timeoutMs > 0 && waitMs >= task.timeoutMs) {
                        if (task.onComplete) { try { task.onComplete(nlohmann::json({{"error","rate_limited"}})); } catch(...){} }
                        continue;
                    }
                    queue_.push(task);
                    continue;
                }
            }
        }

        nlohmann::json lastError;
        bool success = false;
        int totalAttempts = std::max(1, task.retryLimit + 1);
        for (int attempt = 1; attempt <= totalAttempts && running_; ++attempt) {
            task.attempt = attempt;
            try {
                if (task.timeoutMs > 0) {
                    auto fut = std::async(std::launch::async, task.work);
                    if (fut.wait_for(std::chrono::milliseconds(task.timeoutMs)) == std::future_status::ready) {
                        nlohmann::json result = fut.get();
                        if (result.is_object() && result.contains("error")) lastError = result;
                        else {
                            { std::lock_guard<std::mutex> lock(lastCalledMutex_); lastCalledMap_[task.name] = std::chrono::system_clock::now(); }
                            if (task.onComplete) try { task.onComplete(result); } catch(...){}
                            success = true; break;
                        }
                    } else {
                        lastError = nlohmann::json({{"error","timeout"},{"attempt",attempt}});
                    }
                } else {
                    nlohmann::json result = task.work();
                    if (result.is_object() && result.contains("error")) lastError = result;
                    else { std::lock_guard<std::mutex> lock(lastCalledMutex_); lastCalledMap_[task.name] = std::chrono::system_clock::now(); if (task.onComplete) try { task.onComplete(result); } catch(...){} success = true; break; }
                }
            } catch (const std::exception& e) { lastError = nlohmann::json({{"error", std::string("exception: ") + e.what()},{"attempt",attempt}}); }
            catch (...) { lastError = nlohmann::json({{"error","unknown_exception"},{"attempt",attempt}}); }
            if (attempt < totalAttempts) {
                uint32_t backoff = baseBackoffMs * (1u << (attempt - 1));
                if (task.timeoutMs > 0 && backoff > task.timeoutMs) backoff = task.timeoutMs;
                std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
            }
        }
        if (!success) {
            if (task.onComplete) { try { task.onComplete(lastError.is_null() ? nlohmann::json({{"error","failed"}}) : lastError); } catch(...){} }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

} // namespace DecisionEngine
