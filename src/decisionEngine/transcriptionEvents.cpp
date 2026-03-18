/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/

#include "transcriptionEvents.h"

namespace DecisionEngine {

std::mutex TranscriptionEvents::mtx_;
std::unordered_map<TranscriptionEvents::Handle, TranscriptionEvents::Callback> TranscriptionEvents::cbs_;
std::atomic<TranscriptionEvents::Handle> TranscriptionEvents::next_{1};

TranscriptionEvents::Handle TranscriptionEvents::subscribe(Callback cb)
{
    std::lock_guard<std::mutex> lk(mtx_);
    Handle h = next_++;
    cbs_[h] = std::move(cb);
    return h;
}

bool TranscriptionEvents::unsubscribe(Handle h)
{
    std::lock_guard<std::mutex> lk(mtx_);
    return cbs_.erase(h) > 0;
}

void TranscriptionEvents::publish(const std::string& text, bool isFinal)
{
    std::unordered_map<Handle, Callback> copy;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        copy = cbs_;
    }
    for (auto & kv : copy) {
        try { kv.second(text, isFinal); } catch (...) {}
    }
}

} // namespace DecisionEngine

