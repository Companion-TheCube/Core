/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/
#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <atomic>

namespace DecisionEngine {

class TranscriptionEvents {
public:
    using Handle = size_t;
    using Callback = std::function<void(const std::string& text, bool isFinal)>;

    static Handle subscribe(Callback cb);
    static bool unsubscribe(Handle h);
    static void publish(const std::string& text, bool isFinal);

private:
    static std::mutex mtx_;
    static std::unordered_map<Handle, Callback> cbs_;
    static std::atomic<Handle> next_;
};

} // namespace DecisionEngine

