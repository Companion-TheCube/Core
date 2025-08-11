/*
███████╗██╗   ██╗███╗   ██╗ ██████╗████████╗██╗ ██████╗ ███╗   ██╗██████╗ ███████╗ ██████╗ ██╗███████╗████████╗██████╗ ██╗   ██╗██╗  ██╗
██╔════╝██║   ██║████╗  ██║██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║██╔══██╗██╔════╝██╔════╝ ██║██╔════╝╚══██╔══╝██╔══██╗╚██╗ ██╔╝██║  ██║
█████╗  ██║   ██║██╔██╗ ██║██║        ██║   ██║██║   ██║██╔██╗ ██║██████╔╝█████╗  ██║  ███╗██║███████╗   ██║   ██████╔╝ ╚████╔╝ ███████║
██╔══╝  ██║   ██║██║╚██╗██║██║        ██║   ██║██║   ██║██║╚██╗██║██╔══██╗██╔══╝  ██║   ██║██║╚════██║   ██║   ██╔══██╗  ╚██╔╝  ██╔══██║
██║     ╚██████╔╝██║ ╚████║╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║██║  ██║███████╗╚██████╔╝██║███████║   ██║   ██║  ██║   ██║██╗██║  ██║
╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝   ╚═╝╚═╝╚═╝  ╚═╝
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


// Function Registry: structured catalogue of callable functions for tool-use
// by LLMs and internal components.
//
// Goals
// - Allow apps to register discoverable functions with typed parameters
// - Provide a JSON catalogue suitable for LLM tool schemas (OpenAI-style)
// - Offer fast lookup and execution routing by function name
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <regex>
#include <functional>
#include <thread>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <mutex>
#include <nlohmann/json.hpp>
#include <httplib.h>
#ifndef LOGGER_H
#include <logger.h>
#endif
#include "./../apps/appsManager.h"

namespace DecisionEngine {

// Parameter specification (name, type, description, required)
struct ParamSpec {
    std::string name;
    // type must be one of: "string", "number", "boolean", "json"
    std::string type;
    std::string description;
    bool        required{true};
};

// Function specification and callback
struct FunctionSpec {
    std::string name;
    std::string description;
    std::vector<ParamSpec> parameters;
    std::function<nlohmann::json(const nlohmann::json& args)> callback;
    uint32_t timeoutMs{4000};

    // Serialize into OpenAI-style JSON for the LLM
    nlohmann::json toJson() const;
};

// Singleton registry: stores FunctionSpec by name and exposes a JSON catalogue
class FunctionRegistry {
public:
    static FunctionRegistry& instance();

    bool registerFunc(const FunctionSpec& spec);
    const FunctionSpec* find(const std::string& name) const;
    std::vector<nlohmann::json> catalogueJson() const;

private:
    std::unordered_map<std::string, FunctionSpec> funcs_;
    mutable std::mutex mutex_;
};

namespace FunctionUtils {
    // Helper to convert a JSON object to a FunctionSpec
    FunctionSpec fromJson(const nlohmann::json& j) {
        FunctionSpec spec;
        spec.name = j.at("name").get<std::string>();
        spec.description = j.value("description", "");
        spec.timeoutMs = j.value("timeout_ms", 4000);
        for (const auto& param : j.at("parameters")) {
            ParamSpec p;
            p.name = param.at("name").get<std::string>();
            p.type = param.at("type").get<std::string>();
            p.description = param.value("description", "");
            p.required = param.value("required", true);
            spec.parameters.push_back(p);
        }
        if (j.contains("callback")) {
            spec.callback = [j](const nlohmann::json& args) -> nlohmann::json {
                // Placeholder for actual callback logic
                // TODO: Implement the actual function logic here
                CubeLog::error("Function callback not implemented for: " + j.at("name").get<std::string>());
                CubeLog::error("Function args: " + args.dump());
                // Return an empty JSON object or handle as needed
                // This is where you would implement the actual function logic
                // For now, we just log an error and return an empty object
                return nlohmann::json();

                // args is a JSON object containing the parameters
                // You can access parameters like args["param_name"]

                // j.callback contains the function logic
                // functions can be 
            };
        } else {
            spec.callback = [](const nlohmann::json& args) -> nlohmann::json {
                CubeLog::error("No callback defined for function");
                return nlohmann::json();
            };
        }
        return spec;
    }
}


}
