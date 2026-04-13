/*
███████╗██╗   ██╗███████╗███╗   ██╗████████╗███████╗ █████╗ ██████╗ ██╗   ██╗  ██╗
██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██╔════╝██╔══██╗██╔══██╗██║   ██║  ██║
█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ███████╗███████║██████╔╝██║   ███████║
██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ╚════██║██╔══██║██╔═══╝ ██║   ██╔══██║
███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║   ███████║██║  ██║██║     ██║██╗██║  ██║
╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC
*/

#pragma once
#ifndef EVENTS_API_H
#define EVENTS_API_H

#include "../api/api.h"
#include "apiEventBroker.h"

#include <memory>

class EventsAPI : public AutoRegisterAPI<EventsAPI> {
public:
    explicit EventsAPI(std::shared_ptr<API> api);

    std::string getInterfaceName() const override { return "Events"; }
    HttpEndPointData_t getHttpEndpointData() override;

private:
    std::shared_ptr<API> api_;
};

#endif
