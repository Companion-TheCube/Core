/*
████████╗██████╗  █████╗ ███╗   ██╗███████╗ ██████╗██████╗ ██╗██████╗ ████████╗██╗ ██████╗ ███╗   ██╗███████╗██╗   ██╗███████╗███╗   ██╗████████╗███████╗   ██╗  ██╗
╚══██╔══╝██╔══██╗██╔══██╗████╗  ██║██╔════╝██╔════╝██╔══██╗██║██╔══██╗╚══██╔══╝██║██╔═══██╗████╗  ██║██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██╔════╝   ██║  ██║
   ██║   ██████╔╝███████║██╔██╗ ██║███████╗██║     ██████╔╝██║██████╔╝   ██║   ██║██║   ██║██╔██╗ ██║█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ███████╗   ███████║
   ██║   ██╔══██╗██╔══██║██║╚██╗██║╚════██║██║     ██╔══██╗██║██╔═══╝    ██║   ██║██║   ██║██║╚██╗██║██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ╚════██║   ██╔══██║
   ██║   ██║  ██║██║  ██║██║ ╚████║███████║╚██████╗██║  ██║██║██║        ██║   ██║╚██████╔╝██║ ╚████║███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║   ███████║██╗██║  ██║
   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝ ╚═════╝╚═╝  ╚═╝╚═╝╚═╝        ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License
Copyright (c) 2026 A-McD Technology LLC
*/
#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <atomic>

namespace DecisionEngine {

struct TranscriptionEvent {
    std::string fullText;
    std::string appendText;
    bool isFinal = false;
};

class TranscriptionEvents {
public:
    using Handle = size_t;
    using Callback = std::function<void(const TranscriptionEvent&)>;

    static Handle subscribe(Callback cb);
    static bool unsubscribe(Handle h);
    static void publish(const TranscriptionEvent& event);

private:
    static std::mutex mtx_;
    static std::unordered_map<Handle, Callback> cbs_;
    static std::atomic<Handle> next_;
};

} // namespace DecisionEngine
