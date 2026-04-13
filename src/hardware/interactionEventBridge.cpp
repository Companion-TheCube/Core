/*
██╗███╗   ██╗████████╗███████╗██████╗  █████╗  ██████╗████████╗██╗ ██████╗ ███╗   ██╗███████╗██╗   ██╗███████╗███╗   ██╗████████╗██████╗ ██████╗ ██╗██████╗  ██████╗ ███████╗    ██████╗██████╗ ██████╗
██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗██╔══██╗██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██╔══██╗██╔══██╗██║██╔══██╗██╔════╝ ██╔════╝   ██╔════╝██╔══██╗██╔══██╗
██║██╔██╗ ██║   ██║   █████╗  ██████╔╝███████║██║        ██║   ██║██║   ██║██╔██╗ ██║█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ██████╔╝██████╔╝██║██║  ██║██║  ███╗█████╗     ██║     ██████╔╝██████╔╝
██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗██╔══██║██║        ██║   ██║██║   ██║██║╚██╗██║██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ██╔══██╗██╔══██╗██║██║  ██║██║   ██║██╔══╝     ██║     ██╔═══╝ ██╔═══╝
██║██║ ╚████║   ██║   ███████╗██║  ██║██║  ██║╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║   ██████╔╝██║  ██║██║██████╔╝╚██████╔╝███████╗██╗╚██████╗██║     ██║
╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═════╝ ╚═╝  ╚═╝╚═╝╚═════╝  ╚═════╝ ╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC
*/

#include "interactionEventBridge.h"

namespace {

nlohmann::json accelerationSampleToJson(const std::optional<Bmi270AccelerationSample>& sample)
{
    if (!sample.has_value()) {
        return nullptr;
    }

    return nlohmann::json {
        { "xG", sample->xG },
        { "yG", sample->yG },
        { "zG", sample->zG },
    };
}

} // namespace

InteractionEventBridge::InteractionEventBridge(std::shared_ptr<ApiEventBroker> broker)
    : broker_(std::move(broker))
{
    if (!broker_) {
        return;
    }

    broker_->registerSource("interaction");
    handle_ = InteractionEvents::subscribe([broker = broker_](const InteractionEvent& event) {
        if (!broker) {
            return;
        }

        broker->publish(
            "interaction",
            interactionEventTypeToString(event.type),
            nlohmann::json {
                { "liftStateAfter", interactionLiftStateToString(event.liftStateAfter) },
                { "sample", accelerationSampleToJson(event.sample) },
            },
            event.occurredAtEpochMs);
    });
}

InteractionEventBridge::~InteractionEventBridge()
{
    if (handle_ != 0) {
        InteractionEvents::unsubscribe(handle_);
    }
}
