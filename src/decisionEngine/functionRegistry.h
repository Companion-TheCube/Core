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

namespace DecisionEngine {

// Parameter specification (name, type, description, required)
struct ParamSpec {
    std::string name;
    std::string type;          // "string", "number", "boolean", "object", ...
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

    // Serialise into OpenAI-style JSON for the LLM
    nlohmann::json toJson() const;
};

// Singleton registry: stores FunctionSpec by name and exposes a JSON catalogue
class FunctionRegistry {
public:
    static FunctionRegistry& instance();

    void registerFunc(const FunctionSpec& spec);
    const FunctionSpec* find(const std::string& name) const;
    std::vector<nlohmann::json> catalogueJson() const;

private:
    std::unordered_map<std::string, FunctionSpec> funcs_;
};

}
