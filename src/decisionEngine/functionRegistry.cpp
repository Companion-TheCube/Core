/*
███████╗██╗   ██╗███╗   ██╗ ██████╗████████╗██╗ ██████╗ ███╗   ██╗██████╗ ███████╗ ██████╗ ██╗███████╗████████╗██████╗ ██╗   ██╗ ██████╗██████╗ ██████╗ 
██╔════╝██║   ██║████╗  ██║██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║██╔══██╗██╔════╝██╔════╝ ██║██╔════╝╚══██╔══╝██╔══██╗╚██╗ ██╔╝██╔════╝██╔══██╗██╔══██╗
█████╗  ██║   ██║██╔██╗ ██║██║        ██║   ██║██║   ██║██╔██╗ ██║██████╔╝█████╗  ██║  ███╗██║███████╗   ██║   ██████╔╝ ╚████╔╝ ██║     ██████╔╝██████╔╝
██╔══╝  ██║   ██║██║╚██╗██║██║        ██║   ██║██║   ██║██║╚██╗██║██╔══██╗██╔══╝  ██║   ██║██║╚════██║   ██║   ██╔══██╗  ╚██╔╝  ██║     ██╔═══╝ ██╔═══╝ 
██║     ╚██████╔╝██║ ╚████║╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║██║  ██║███████╗╚██████╔╝██║███████║   ██║   ██║  ██║   ██║██╗╚██████╗██║     ██║     
╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝   ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
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


/*

The function registry provides a way to register and manage functions, which are data sources that are provided by apps.
Functions can be registered with a name, action, parameters, and other metadata.

It also provides HTTP endpoints for interacting with the function registry, allowing other applications to register, unregister, and query functions.
The decision engine can execute functions based on user input or other triggers, and it can also manage scheduled tasks that execute functions at 
specified times or intervals.

Functions are registered with the following information:
- Function name
- Action to execute
- Parameters for the action. These can be static or can be loaded from a data source.
- Brief description of the function
- Response string that can include placeholders for parameters
- Emotional score ranges for the function to be enabled or disabled based on the user's emotional state
- Matching params for Spacy
- Matching phrases for other matching methods

Functions are called via RPC. We'll use a simple JSON-RPC style interface for this. Each app will get a unix socket in the root of its directory 
that it should use for RPC. 

*/

#include "functionRegistry.h"

namespace DecisionEngine {

namespace FunctionUtils {
    // Helper to convert a JSON object to a FunctionSpec
    FunctionSpec specFromJson(const nlohmann::json& j);
}

FunctionSpec::FunctionSpec()
    : name(""), appName(""), version(""), description(""), humanReadableName(""),
      timeoutMs(4000), rateLimit(0), retryLimit(3), lastCalled(TimePoint::min()), enabled(true)
{
}

FunctionSpec::FunctionSpec(const FunctionSpec& other)
    : name(other.name), appName(other.appName), version(other.version),
      description(other.description), humanReadableName(other.humanReadableName),
      parameters(other.parameters), callback(other.callback), timeoutMs(other.timeoutMs),
      rateLimit(other.rateLimit), retryLimit(other.retryLimit), lastCalled(other.lastCalled),
      emotionalScoreRanges(other.emotionalScoreRanges), matchingParams(other.matchingParams),
      matchingPhrases(other.matchingPhrases), enabled(other.enabled)
{
}

FunctionSpec& FunctionSpec::operator=(const FunctionSpec& other)
{
    if (this != &other) {
        name = other.name;
        appName = other.appName;
        version = other.version;
        description = other.description;
        humanReadableName = other.humanReadableName;
        parameters = other.parameters;
        callback = other.callback;
        timeoutMs = other.timeoutMs;
        rateLimit = other.rateLimit;
        retryLimit = other.retryLimit;
        lastCalled = other.lastCalled;
        emotionalScoreRanges = other.emotionalScoreRanges;
        matchingParams = other.matchingParams;
        matchingPhrases = other.matchingPhrases;
        enabled = other.enabled;
    }
    return *this;
}

FunctionSpec::FunctionSpec(FunctionSpec&& other) noexcept
    : name(std::move(other.name)), appName(std::move(other.appName)), version(std::move(other.version)),
      description(std::move(other.description)), humanReadableName(std::move(other.humanReadableName)),
      parameters(std::move(other.parameters)), callback(std::move(other.callback)), timeoutMs(other.timeoutMs),
      rateLimit(other.rateLimit), retryLimit(other.retryLimit), lastCalled(std::move(other.lastCalled)),
      emotionalScoreRanges(std::move(other.emotionalScoreRanges)), matchingParams(std::move(other.matchingParams)),
      matchingPhrases(std::move(other.matchingPhrases)), enabled(other.enabled)
{
}

FunctionSpec& FunctionSpec::operator=(FunctionSpec&& other)
{
    if (this != &other) {
        name = std::move(other.name);
        appName = std::move(other.appName);
        version = std::move(other.version);
        description = std::move(other.description);
        humanReadableName = std::move(other.humanReadableName);
        parameters = std::move(other.parameters);
        callback = std::move(other.callback);
        timeoutMs = other.timeoutMs;
        rateLimit = other.rateLimit;
        retryLimit = other.retryLimit;
        lastCalled = std::move(other.lastCalled);
        emotionalScoreRanges = std::move(other.emotionalScoreRanges);
        matchingParams = std::move(other.matchingParams);
        matchingPhrases = std::move(other.matchingPhrases);
        enabled = other.enabled;
    }
    return *this;
}

nlohmann::json FunctionSpec::toJson() const
{
    nlohmann::json j;
    j["name"] = name;
    j["description"] = description;
    j["parameters"] = nlohmann::json::array();
    for (const auto& param : parameters) {
        nlohmann::json paramJson;
        paramJson["name"] = param.name;
        paramJson["type"] = param.type;
        paramJson["description"] = param.description;
        paramJson["required"] = param.required;
        j["parameters"].push_back(paramJson);
    }
    j["timeoutMs"] = timeoutMs;
    return j;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

FunctionRegistry::FunctionRegistry()
{
    // Register the FunctionRegistry with the API builder
    this->registerInterface();
}

FunctionRegistry::~FunctionRegistry()
{
    // Destructor logic if needed
}

bool FunctionRegistry::registerFunc(const FunctionSpec& spec)
{
    // first we verify that the function name is valid. Must start with a version indicator like "v1." and then be a valid identifier.
    // Valid identifiers can only contain alphanumeric characters and underscores, and cannot start with a number.
    // The next part of the name must be the app name, which must be a valid identifier.
    // The last part of the name must be the function name, which must also be a valid identifier.
    // Example: "v1.my_app.my_function" is valid, but "v1.1myapp.my_function" is not valid.
    // The version must be a valid semantic version, e.g. "v1_0_0", "v2_1_3", etc. Version indicates the minimum version of the CORE
    // that the app requires.
    // valid examples: "v1_0_0.my_app.my_function", "v2_1_3.my_app.my_function"
    if(spec.name.empty()){
        CubeLog::error("Function name cannot be empty");
        return false;
    }
    if (!std::regex_match(spec.name, std::regex("^v[0-9_]+\\.[a-zA-Z_][a-zA-Z0-9_]\\.[a-zA-Z_][a-zA-Z0-9_]*$"))) {
        // first we check that the version is valid
        if(!std::regex_match(spec.name, std::regex("^v[0-9_]+\\.[a-zA-Z_\\.0-9]*$"))) {
            CubeLog::error("Function name must start with a valid version indicator like 'v1.0.0.my_app.my_function'");
            return false;
        }
        // then we check that the app name is valid
        // get the app name from the function name
        std::string appName = spec.name.substr(spec.name.find('.') + 1, spec.name.find('.', spec.name.find('.') + 1) - spec.name.find('.') - 1);
        // then check that the app name is a valid identifier
        if (!std::regex_match(appName, std::regex("^[a-zA-Z_][a-zA-Z0-9_]*$"))) {
            CubeLog::error("App name '" + appName + "' in function name '" + spec.name + "' is not a valid identifier. It must start with a letter or underscore and can only contain alphanumeric characters and underscores.");
            return false;
        }
        // then check that the app name is in the DB
        auto appNames = AppsManager::getAppNames_static();
        if (std::find(appNames.begin(), appNames.end(), appName) == appNames.end()) {
            CubeLog::error("App '" + appName + "' is not registered. Please register the app before registering functions.");
            return false;
        }
        // then we check that the function name is valid
        std::string functionName = spec.name.substr(spec.name.find_last_of('.') + 1);
        if (!std::regex_match(functionName, std::regex("^[a-zA-Z_][a-zA-Z0-9_]*$"))) {
            CubeLog::error("Function name '" + functionName + "' in function name '" + spec.name + "' is not a valid identifier. It must start with a letter or underscore and can only contain alphanumeric characters and underscores.");
            return false;
        }
    }
    std::lock_guard<std::mutex> lock(mutex_);
    funcs_[spec.name] = spec;
    CubeLog::info("Registered function: " + spec.name);
    return true;
}

const FunctionSpec* FunctionRegistry::find(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = funcs_.find(name);
    if (it != funcs_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<nlohmann::json> FunctionRegistry::catalogueJson() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<nlohmann::json> catalogue;
    for (const auto& [name, spec] : funcs_) {
        catalogue.push_back(spec.toJson());
    }
    return catalogue;
}

HttpEndPointData_t FunctionRegistry::getHttpEndpointData()
{
    HttpEndPointData_t endpoints;
    endpoints.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res){
            // Register a function
            nlohmann::json j;
            try {
                j = nlohmann::json::parse(req.body);
            } catch (const nlohmann::json::parse_error& e) {
                res.status = 400;
                res.set_content("Invalid JSON: " + std::string(e.what()), "text/plain");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_REQUEST, "Invalid JSON format");
            }

            FunctionSpec spec = FunctionUtils::specFromJson(j);
            if (registerFunc(spec)) {
                res.status = 200;
                res.set_content("Function registered successfully", "text/plain");
            } else {
                res.status = 400;
                res.set_content("Failed to register function", "text/plain");
            }
            res.set_header("Content-Type", "application/json");
            res.body = j.dump();
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR,"");
        },
        "Register a function", {}, "Register a new function with the function registry" });
    endpoints.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res){
            // Get function catalogue
            nlohmann::json catalogue = catalogueJson();
            res.status = 200;
            res.set_content(catalogue.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR,"");
        },
        "Get function catalogue", {}, "Get the list of registered functions" });
    endpoints.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res){
            // Find a function by name
            std::string functionName = req.get_param_value("name");
            if (functionName.empty()) {
                res.status = 400;
                res.set_content("Function name is required", "text/plain");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "Function name is required");
            }
            const FunctionSpec* func = find(functionName);
            if (func) {
                nlohmann::json j = func->toJson();
                res.status = 200;
                res.set_content(j.dump(), "application/json");
            } else {
                res.status = 404;
                res.set_content("Function not found", "text/plain");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NOT_FOUND, "Function not found");
            }
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR,"");
        },
        "Find a function by name", {}, "Find a registered function by its name" });
    endpoints.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res){
            // Unregister a function
            std::string functionName = req.get_param_value("name");
            if (functionName.empty()) {
                res.status = 400;
                res.set_content("Function name is required", "text/plain");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "Function name is required");
            }
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = funcs_.find(functionName);
            if (it != funcs_.end()) {
                funcs_.erase(it);
                res.status = 200;
                res.set_content("Function unregistered successfully", "text/plain");
            } else {
                res.status = 404;
                res.set_content("Function not found", "text/plain");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NOT_FOUND, "Function not found");
            }
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR,"");
        },
        "Unregister a function", {}, "Unregister a function from the function registry" });
    endpoints.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res){
            // Get the list of all registered functions
            nlohmann::json catalogue = catalogueJson();
            res.status = 200;
            res.set_content(catalogue.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR,"");
        },
        "Get all registered functions", {}, "Get the list of all registered functions" });
    return endpoints;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace FunctionUtils {
    // Helper to convert a JSON object to a FunctionSpec
    FunctionSpec specFromJson(const nlohmann::json& j)
    {
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

                // Here we need to generate the call to the json-rpc IPC service. This is not implemented yet.
            };
        } else {
            spec.callback = [](const nlohmann::json& args) -> nlohmann::json {
                CubeLog::error("No callback defined for function");
                return nlohmann::json();
            };
        }
        spec.appName = j.value("app_name", "");
        spec.version = j.value("version", "1.0");
        spec.humanReadableName = j.value("human_readable_name", "");
        spec.rateLimit = j.value("rate_limit", 0);
        spec.retryLimit = j.value("retry_limit", 3);
        spec.lastCalled = TimePoint::min();
        spec.enabled = j.value("enabled", true);
        if (j.contains("emotional_score_ranges")) {
            for (const auto& range : j.at("emotional_score_ranges")) {
                if (range.is_array() && range.size() == 2) {
                    int minScore = range.at(0).get<int>();
                    int maxScore = range.at(1).get<int>();
                    spec.emotionalScoreRanges.emplace_back(minScore, maxScore);
                } else {
                    CubeLog::error("Invalid emotional score range format in function spec: " + j.at("name").get<std::string>());
                }
            }
        }
        if (j.contains("matching_params")) {
            for (const auto& param : j.at("matching_params")) {
                spec.matchingParams.push_back(param.get<std::string>());
            }
        }
        if (j.contains("matching_phrases")) {
            for (const auto& phrase : j.at("matching_phrases")) {
                spec.matchingPhrases.push_back(phrase.get<std::string>());
            }
        }
        return spec;
    }
}

}