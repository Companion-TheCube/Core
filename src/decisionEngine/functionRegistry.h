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

struct ParamSpec {
    std::string name;
    std::string type;          // "string", "number", "boolean", "object", ...
    std::string description;
    bool        required{true};
};

struct FunctionSpec {
    std::string name;
    std::string description;
    std::vector<ParamSpec> parameters;
    std::function<nlohmann::json(const nlohmann::json& args)> callback;
    uint32_t timeoutMs{4000};

    // Serialise into OpenAI-style JSON for the LLM
    nlohmann::json toJson() const;
};

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