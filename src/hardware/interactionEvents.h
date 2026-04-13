/*
██╗███╗   ██╗████████╗███████╗██████╗  █████╗  ██████╗████████╗██╗ ██████╗ ███╗   ██╗
██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗██╔══██╗██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║
██║██╔██╗ ██║   ██║   █████╗  ██████╔╝███████║██║        ██║   ██║██║   ██║██╔██╗ ██║
██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗██╔══██║██║        ██║   ██║██║   ██║██║╚██╗██║
██║██║ ╚████║   ██║   ███████╗██║  ██║██║  ██║╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║
╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝
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
#ifndef INTERACTION_EVENTS_H
#define INTERACTION_EVENTS_H

#include "accel.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

enum class InteractionEventType : uint8_t {
    Tap = 0,
    LiftStarted,
    LiftEnded,
};

struct InteractionEvent {
    uint64_t sequence = 0;
    InteractionEventType type = InteractionEventType::Tap;
    uint64_t occurredAtEpochMs = 0;
    Bmi270LiftState liftStateAfter = Bmi270LiftState::Unknown;
    std::optional<Bmi270AccelerationSample> sample;
};

struct InteractionEventPage {
    std::vector<InteractionEvent> events;
    uint64_t nextSequence = 1;
    bool historyTruncated = false;
};

const char* interactionEventTypeToString(InteractionEventType type);
const char* interactionLiftStateToString(Bmi270LiftState state);

class InteractionEvents {
public:
    using Handle = size_t;
    using Callback = std::function<void(const InteractionEvent&)>;

    static Handle subscribe(Callback cb);
    static bool unsubscribe(Handle handle);
    static void publish(const InteractionEvent& event);

private:
    static std::mutex mutex_;
    static std::unordered_map<Handle, Callback> callbacks_;
    static std::atomic<Handle> nextHandle_;
};

#endif
