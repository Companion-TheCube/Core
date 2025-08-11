/*
███████╗██╗   ██╗███╗   ██╗ ██████╗████████╗██╗ ██████╗ ███╗   ██╗        ██╗   ██╗████████╗██╗██╗     ███████╗        ██╗███╗   ███╗██████╗ ██╗         ██████╗██████╗ ██████╗ 
██╔════╝██║   ██║████╗  ██║██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║        ██║   ██║╚══██╔══╝██║██║     ██╔════╝        ██║████╗ ████║██╔══██╗██║        ██╔════╝██╔══██╗██╔══██╗
█████╗  ██║   ██║██╔██╗ ██║██║        ██║   ██║██║   ██║██╔██╗ ██║        ██║   ██║   ██║   ██║██║     ███████╗        ██║██╔████╔██║██████╔╝██║        ██║     ██████╔╝██████╔╝
██╔══╝  ██║   ██║██║╚██╗██║██║        ██║   ██║██║   ██║██║╚██╗██║        ██║   ██║   ██║   ██║██║     ╚════██║        ██║██║╚██╔╝██║██╔═══╝ ██║        ██║     ██╔═══╝ ██╔═══╝ 
██║     ╚██████╔╝██║ ╚████║╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║███████╗╚██████╔╝   ██║   ██║███████╗███████║███████╗██║██║ ╚═╝ ██║██║     ███████╗██╗╚██████╗██║     ██║     
╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝ ╚═════╝    ╚═╝   ╚═╝╚══════╝╚══════╝╚══════╝╚═╝╚═╝     ╚═╝╚═╝     ╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
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


#include "functionRegistry.h"

namespace DecisionEngine {
namespace FunctionUtils {
    FunctionSpec specFromJson(const nlohmann::json& j) {
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
        spec.onComplete = [j](const nlohmann::json& result) {
            CubeLog::error("Function onComplete not implemented for: " + j.at("name").get<std::string>());
            CubeLog::debug("Result: " + result.dump());
        };
        spec.appName = j.value("app_name", "");
        spec.version = j.value("version", "1.0");
        spec.humanReadableName = j.value("human_readable_name", "");
        spec.rateLimit = j.value("rate_limit", 0);
        spec.retryLimit = j.value("retry_limit", 3);
        spec.lastCalled = TimePoint::min();
        spec.enabled = j.value("enabled", true);
        return spec;
    }

    CapabilitySpec capabilityFromJson(const nlohmann::json& j) {
        CapabilitySpec spec;
        spec.name = j.value("name", "");
        spec.description = j.value("description", "");
        spec.timeoutMs = j.value("timeout_ms", 2000);
        spec.retryLimit = j.value("retry_limit", 0);
        spec.enabled = j.value("enabled", true);
        spec.type = j.value("type", "core");
        spec.entry = j.value("entry", "");
        if (j.contains("parameters")) {
            for (const auto& param : j.at("parameters")) {
                ParamSpec p;
                p.name = param.at("name").get<std::string>();
                p.type = param.at("type").get<std::string>();
                p.description = param.value("description", "");
                p.required = param.value("required", true);
                spec.parameters.push_back(p);
            }
        }
        spec.action = nullptr;
        return spec;
    }
}
}

