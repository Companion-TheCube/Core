#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#ifndef _WIN32
#include <unistd.h>
extern char** environ;
#endif

// TODO: make sure we are using landlock to sandbox the launched applications, and that the launch policies are configured to only allow the necessary permissions for each application.

namespace {
namespace fs = std::filesystem;

constexpr const char* kDefaultLaunchRoot = "/run/thecube/launch";
constexpr const char* kDefaultLocalLaunchRoot = "run/thecube/launch";

bool shouldUseSystemPathsByDefault()
{
#ifdef _WIN32
    return false;
#else
    return geteuid() == 0;
#endif
}

fs::path currentWorkingDirectoryOrFallback(const fs::path& fallback)
{
#ifndef _WIN32
    std::error_code execEc;
    const auto executablePath = fs::read_symlink("/proc/self/exe", execEc);
    if (!execEc && !executablePath.empty()) {
        return executablePath.parent_path();
    }
#endif

    std::error_code ec;
    const auto cwd = fs::current_path(ec);
    if (ec) {
        return fallback;
    }
    return cwd;
}

fs::path configuredBasePath(const char* envKey, const char* systemDefault, const char* localDefault)
{
    if (const auto* configured = std::getenv(envKey); configured != nullptr && *configured != '\0') {
        fs::path path(configured);
        std::error_code ec;
        if (path.is_relative()) {
            path = fs::absolute(path, ec);
            if (ec) {
                return path.lexically_normal();
            }
        }
        return path.lexically_normal();
    }

    if (shouldUseSystemPathsByDefault()) {
        return fs::path(systemDefault);
    }

    return (currentWorkingDirectoryOrFallback(fs::path(".")) / localDefault).lexically_normal();
}

std::map<std::string, std::string> captureEnvironment()
{
    std::map<std::string, std::string> values;
#ifndef _WIN32
    for (char** cursor = environ; cursor != nullptr && *cursor != nullptr; ++cursor) {
        const std::string entry(*cursor);
        const auto separator = entry.find('=');
        if (separator == std::string::npos) {
            continue;
        }
        values.emplace(entry.substr(0, separator), entry.substr(separator + 1));
    }
#endif
    return values;
}

bool applyEnvironment(const nlohmann::json& policy, std::string& error)
{
#ifdef _WIN32
    (void)policy;
    error = "CubeAppLauncher only supports Linux in v1";
    return false;
#else
    const auto existing = captureEnvironment();

    const auto environment = policy.value("environment", nlohmann::json::object());
    const bool clearInherited = environment.value("clear_inherited", true);
    if (clearInherited && clearenv() != 0) {
        error = std::string("clearenv failed: ") + std::strerror(errno);
        return false;
    }

    if (environment.contains("allow_inherit")) {
        if (!environment["allow_inherit"].is_array()) {
            error = "environment.allow_inherit must be an array";
            return false;
        }
        for (const auto& entry : environment["allow_inherit"]) {
            if (!entry.is_string()) {
                error = "environment.allow_inherit entries must be strings";
                return false;
            }
            const auto key = entry.get<std::string>();
            const auto found = existing.find(key);
            if (found != existing.end() && setenv(found->first.c_str(), found->second.c_str(), 1) != 0) {
                error = "setenv failed for inherited variable " + found->first + ": " + std::strerror(errno);
                return false;
            }
        }
    }

    if (environment.contains("set")) {
        if (!environment["set"].is_object()) {
            error = "environment.set must be an object";
            return false;
        }
        for (auto it = environment["set"].begin(); it != environment["set"].end(); ++it) {
            if (!it.value().is_string()) {
                error = "environment.set values must be strings";
                return false;
            }
            const auto value = it.value().get<std::string>();
            if (setenv(it.key().c_str(), value.c_str(), 1) != 0) {
                error = "setenv failed for " + it.key() + ": " + std::strerror(errno);
                return false;
            }
        }
    }

    return true;
#endif
}

fs::path resolvePolicyPath(const std::string& appId, const std::optional<fs::path>& explicitLaunchRoot)
{
    if (explicitLaunchRoot.has_value()) {
        return (*explicitLaunchRoot / appId / "launch-policy.json").lexically_normal();
    }

    return (configuredBasePath("THECUBE_LAUNCH_ROOT", kDefaultLaunchRoot, kDefaultLocalLaunchRoot) / appId / "launch-policy.json").lexically_normal();
}

std::optional<nlohmann::json> loadPolicy(const fs::path& policyPath, std::string& error)
{
    std::ifstream input(policyPath);
    if (!input.is_open()) {
        error = "Unable to open launch policy: " + policyPath.string();
        return std::nullopt;
    }

    try {
        nlohmann::json policy;
        input >> policy;
        if (!policy.is_object()) {
            error = "Launch policy root must be an object";
            return std::nullopt;
        }
        return policy;
    } catch (const std::exception& ex) {
        error = "Failed to parse launch policy: " + std::string(ex.what());
        return std::nullopt;
    }
}

bool collectArgv(const nlohmann::json& policy, std::vector<std::string>& argvOut, std::string& error)
{
    if (!policy.contains("app") || !policy["app"].is_object()) {
        error = "Launch policy is missing app object";
        return false;
    }
    if (!policy["app"].contains("argv") || !policy["app"]["argv"].is_array()) {
        error = "Launch policy app.argv must be an array";
        return false;
    }

    for (const auto& entry : policy["app"]["argv"]) {
        if (!entry.is_string()) {
            error = "Launch policy app.argv entries must be strings";
            return false;
        }
        argvOut.push_back(entry.get<std::string>());
    }

    if (argvOut.empty()) {
        error = "Launch policy app.argv must not be empty";
        return false;
    }

    return true;
}
} // namespace

int main(int argc, char* argv[])
{
#ifndef __linux__
    std::cerr << "CubeAppLauncher only supports Linux in v1.\n";
    return 1;
#endif

    std::optional<fs::path> explicitLaunchRoot;
    std::optional<fs::path> explicitPolicyPath;
    std::string appId;

    for (int index = 1; index < argc; ++index) {
        const std::string argument(argv[index]);
        if (argument == "--launch-root" && index + 1 < argc) {
            explicitLaunchRoot = fs::path(argv[++index]);
            continue;
        }
        if (argument == "--policy" && index + 1 < argc) {
            explicitPolicyPath = fs::path(argv[++index]);
            continue;
        }
        if (appId.empty()) {
            appId = argument;
            continue;
        }

        std::cerr << "Unexpected argument: " << argument << "\n";
        return 2;
    }

    if (explicitPolicyPath.has_value() == false && appId.empty()) {
        std::cerr << "Usage: CubeAppLauncher <app-id> [--launch-root <path>] or CubeAppLauncher --policy <path>\n";
        return 2;
    }

    fs::path policyPath;
    if (explicitPolicyPath.has_value()) {
        policyPath = explicitPolicyPath->lexically_normal();
    } else {
        policyPath = resolvePolicyPath(appId, explicitLaunchRoot);
    }

    std::string error;
    auto policy = loadPolicy(policyPath, error);
    if (!policy.has_value()) {
        std::cerr << error << "\n";
        return 3;
    }

    std::vector<std::string> launchArgv;
    if (!collectArgv(*policy, launchArgv, error)) {
        std::cerr << error << "\n";
        return 4;
    }

#ifdef _WIN32
    std::cerr << "CubeAppLauncher only supports Linux in v1.\n";
    return 1;
#else
    const auto workingDirectory = (*policy)["app"].value("working_directory", std::string());
    if (!workingDirectory.empty() && chdir(workingDirectory.c_str()) != 0) {
        std::cerr << "Failed to change working directory to " << workingDirectory << ": " << std::strerror(errno) << "\n";
        return 5;
    }

    if (!applyEnvironment(*policy, error)) {
        std::cerr << error << "\n";
        return 6;
    }

    std::vector<char*> execArgv;
    execArgv.reserve(launchArgv.size() + 1);
    for (auto& value : launchArgv) {
        execArgv.push_back(value.data());
    }
    execArgv.push_back(nullptr);

    execv(execArgv[0], execArgv.data());
    std::cerr << "execv failed for " << execArgv[0] << ": " << std::strerror(errno) << "\n";
    return 7;
#endif
}
