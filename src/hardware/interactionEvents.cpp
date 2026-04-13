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
*/

#include "interactionEvents.h"

std::mutex InteractionEvents::mutex_;
std::unordered_map<InteractionEvents::Handle, InteractionEvents::Callback> InteractionEvents::callbacks_;
std::atomic<InteractionEvents::Handle> InteractionEvents::nextHandle_ { 1 };

const char* interactionEventTypeToString(InteractionEventType type)
{
    switch (type) {
    case InteractionEventType::Tap:
        return "tap";
    case InteractionEventType::LiftStarted:
        return "lift_started";
    case InteractionEventType::LiftEnded:
        return "lift_ended";
    default:
        return "unknown";
    }
}

const char* interactionLiftStateToString(Bmi270LiftState state)
{
    switch (state) {
    case Bmi270LiftState::OnDesk:
        return "on_desk";
    case Bmi270LiftState::Lifted:
        return "lifted";
    case Bmi270LiftState::Unknown:
    default:
        return "unknown";
    }
}

InteractionEvents::Handle InteractionEvents::subscribe(Callback cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const Handle handle = nextHandle_++;
    callbacks_[handle] = std::move(cb);
    return handle;
}

bool InteractionEvents::unsubscribe(Handle handle)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return callbacks_.erase(handle) > 0;
}

void InteractionEvents::publish(const InteractionEvent& event)
{
    std::unordered_map<Handle, Callback> callbacksCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacksCopy = callbacks_;
    }

    for (const auto& [handle, callback] : callbacksCopy) {
        (void)handle;
        try {
            callback(event);
        } catch (...) {
        }
    }
}
