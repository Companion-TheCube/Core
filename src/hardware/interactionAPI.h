/*
██╗███╗   ██╗████████╗███████╗██████╗  █████╗  ██████╗████████╗██╗ ██████╗ ███╗   ██╗ █████╗ ██████╗ ██╗   ██╗  ██╗
██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗██╔══██╗██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║██╔══██╗██╔══██╗██║   ██║  ██║
██║██╔██╗ ██║   ██║   █████╗  ██████╔╝███████║██║        ██║   ██║██║   ██║██╔██╗ ██║███████║██████╔╝██║   ███████║
██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗██╔══██║██║        ██║   ██║██║   ██║██║╚██╗██║██╔══██║██╔═══╝ ██║   ██╔══██║
██║██║ ╚████║   ██║   ███████╗██║  ██║██║  ██║╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║██║  ██║██║     ██║██╗██║  ██║
╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝     ╚═╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC
*/

#pragma once
#ifndef INTERACTION_API_H
#define INTERACTION_API_H

#include "../api/api.h"
#include "../api/apiEventBroker.h"
#include "peripheralManager.h"

#include <memory>

class InteractionAPI : public AutoRegisterAPI<InteractionAPI> {
public:
    InteractionAPI(
        std::shared_ptr<PeripheralManager> peripheralManager,
        std::shared_ptr<ApiEventBroker> broker);

    std::string getInterfaceName() const override { return "Interaction"; }
    HttpEndPointData_t getHttpEndpointData() override;

private:
    std::shared_ptr<PeripheralManager> peripheralManager_;
    std::shared_ptr<ApiEventBroker> broker_;
};

#endif
