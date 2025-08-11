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

FunctionRegistry& FunctionRegistry::instance()
{
    static FunctionRegistry instance;
    return instance;
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


}