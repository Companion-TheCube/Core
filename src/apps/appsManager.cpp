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

#include "appsManager.h"
#include "appPostgresAccess.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string_view>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/wait.h>
#include <unordered_set>

namespace {
namespace fs = std::filesystem;

constexpr const char* kAppsDatabaseName = "apps";
constexpr const char* kAppsTableName = "apps";
constexpr const char* kDefaultInstallRoot = "/opt/thecube/apps";
constexpr const char* kDefaultRelativeAppsDir = "apps";
constexpr const char* kDefaultLaunchRoot = "/run/thecube/launch";
constexpr const char* kDefaultRuntimeRoot = "/run/thecube/apps";
constexpr const char* kDefaultDataRoot = "/var/lib/thecube/apps";
constexpr const char* kDefaultCacheRoot = "/var/cache/thecube/apps";
constexpr const char* kDefaultLocalLaunchRoot = "run/thecube/launch";
constexpr const char* kDefaultLocalRuntimeRoot = "run/thecube/apps";
constexpr const char* kDefaultLocalDataRoot = "data/apps";
constexpr const char* kDefaultLocalCacheRoot = "cache/apps";
constexpr const char* kDefaultCoreSocket = "/run/thecube/core/core.sock";
constexpr const char* kDefaultLauncherBinaryName = "CubeAppLauncher";
constexpr const char* kDefaultInstalledLauncherPath = "/opt/thecube/bin/CubeAppLauncher";
constexpr const char* kDefaultManagedPostgresAppId = "com.thecube.postgresql";
const std::regex kAppIdPattern("^[a-z0-9.-]+$");

struct ManifestSummary {
    fs::path manifestPath;
    fs::path installRoot;
    std::string schemaVersion;
    std::string appId;
    std::string appName;
    std::string appVersion;
    std::string appType;
    std::string runtimeType;
    std::string runtimeDistribution;
    std::string runtimeCompatibility;
    std::string workingDirectory { "." };
    std::vector<std::string> args;
    std::vector<std::string> postgresqlDatabases;
    bool autostart = false;
    bool systemApp = false;
    nlohmann::json raw;
};

struct ManifestParseResult {
    std::optional<ManifestSummary> manifest;
    std::string error;
};

struct RegistryRow {
    std::string appId;
    std::string appName;
    std::string manifestPath;
    std::string installRoot;
    std::string enabled;
    std::string isSystemApp;
    std::string autostart;
    std::string policyCompileStatus;
    std::string policyCompileError;
    std::string socketLocation;
};

struct PythonInstallState {
    std::string appVersion;
    std::string packageName;
    std::string venvRoot;
};

std::string shellQuote(const std::string& value)
{
    std::string quoted = "'";
    for (char ch : value) {
        if (ch == '\'') {
            quoted += "'\"'\"'";
        } else {
            quoted.push_back(ch);
        }
    }
    quoted += "'";
    return quoted;
}

int extractCommandExitCode(int rc)
{
    if (rc == -1) {
        return -1;
    }
    if (WIFEXITED(rc)) {
        return WEXITSTATUS(rc);
    }
    return rc;
}

std::string isoTimestampUtcNow()
{
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm {};
#ifdef _WIN32
    gmtime_s(&tm, &time);
#else
    gmtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string boolToDb(bool value)
{
    return value ? "1" : "0";
}

bool dbToBool(const std::string& value)
{
    return value == "1" || value == "true" || value == "TRUE" || value == "True";
}

std::string trim(const std::string& input)
{
    const auto begin = input.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = input.find_last_not_of(" \t\r\n");
    return input.substr(begin, end - begin + 1);
}

std::vector<std::string> splitSearchPaths(const std::string& raw)
{
    std::vector<std::string> parts;
    std::string current;
    for (char ch : raw) {
        if (ch == ':' || ch == ';' || ch == ',') {
            const auto value = trim(current);
            if (!value.empty()) {
                parts.push_back(value);
            }
            current.clear();
            continue;
        }
        current.push_back(ch);
    }

    const auto last = trim(current);
    if (!last.empty()) {
        parts.push_back(last);
    }
    return parts;
}

void appendUniqueRoot(std::vector<fs::path>& roots, std::set<std::string>& seen, fs::path root)
{
    if (root.empty()) {
        return;
    }

    std::error_code ec;
    if (root.is_relative()) {
        root = fs::absolute(root, ec);
        if (ec) {
            return;
        }
    }

    const auto normalized = root.lexically_normal();
    if (seen.insert(normalized.string()).second) {
        roots.push_back(normalized);
    }
}

fs::path currentWorkingDirectoryOrFallback(const fs::path& fallback);

std::vector<fs::path> getDefaultInstallRoots()
{
    std::vector<fs::path> roots;
    std::set<std::string> seen;

    appendUniqueRoot(roots, seen, currentWorkingDirectoryOrFallback(fs::path(".")) / kDefaultRelativeAppsDir);

    appendUniqueRoot(roots, seen, fs::path(kDefaultInstallRoot));
    return roots;
}

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

fs::path configuredBasePath(const char* configKey, const char* systemDefault, const char* localDefault)
{
    const auto configured = Config::get(configKey, "");
    if (!configured.empty()) {
        std::error_code ec;
        fs::path path(configured);
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

std::vector<fs::path> getConfiguredInstallRoots()
{
    const auto raw = Config::get("APP_INSTALL_ROOTS", "");
    const auto pieces = splitSearchPaths(raw);
    std::vector<fs::path> roots;
    std::set<std::string> seen;

    if (pieces.empty()) {
        for (const auto& root : getDefaultInstallRoots()) {
            appendUniqueRoot(roots, seen, root);
        }
    } else {
        for (const auto& piece : pieces) {
            appendUniqueRoot(roots, seen, fs::path(piece));
        }
    }

    if (roots.empty()) {
        for (const auto& root : getDefaultInstallRoots()) {
            appendUniqueRoot(roots, seen, root);
        }
    }

    return roots;
}

std::vector<fs::path> discoverManifestPaths()
{
    std::vector<fs::path> manifests;
    std::set<std::string> seen;

    for (const auto& root : getConfiguredInstallRoots()) {
        CubeLog::info("AppsManager: scanning install root " + root.string());
        std::error_code ec;
        if (!fs::exists(root, ec) || ec) {
            CubeLog::warning("AppsManager: install root is unavailable: " + root.string());
            continue;
        }

        const auto rootManifest = root / "manifest.json";
        if (fs::is_regular_file(rootManifest, ec) && !ec) {
            const auto normalized = fs::absolute(rootManifest).lexically_normal().string();
            if (seen.insert(normalized).second) {
                manifests.push_back(fs::absolute(rootManifest).lexically_normal());
                CubeLog::info("AppsManager: discovered root manifest " + normalized);
            }
        }

        std::error_code iterEc;
        const auto options = fs::directory_options::skip_permission_denied;
        for (fs::directory_iterator it(root, options, iterEc), end; it != end; it.increment(iterEc)) {
            if (iterEc) {
                CubeLog::error("AppsManager: error while iterating install root " + root.string() + ": " + iterEc.message());
                break;
            }

            const auto& entry = *it;
            std::error_code entryEc;
            if (!entry.is_directory(entryEc)) {
                if (entryEc) {
                    CubeLog::warning("AppsManager: skipping unreadable entry " + entry.path().string() + ": " + entryEc.message());
                }
                continue;
            }

            const auto manifestPath = entry.path() / "manifest.json";
            std::error_code manifestEc;
            if (!fs::is_regular_file(manifestPath, manifestEc)) {
                if (manifestEc) {
                    CubeLog::warning("AppsManager: failed to inspect manifest candidate " + manifestPath.string() + ": " + manifestEc.message());
                }
                continue;
            }

            const auto normalized = fs::absolute(manifestPath).lexically_normal().string();
            if (seen.insert(normalized).second) {
                manifests.push_back(fs::absolute(manifestPath).lexically_normal());
                CubeLog::info("AppsManager: discovered manifest " + normalized);
            }
        }
    }

    return manifests;
}

bool isPathWithinRoot(const fs::path& candidate, const fs::path& root)
{
    const auto candidateString = candidate.lexically_normal().string();
    const auto rootString = root.lexically_normal().string();
    if (candidateString == rootString) {
        return true;
    }
    if (candidateString.size() <= rootString.size()) {
        return false;
    }
    if (candidateString.rfind(rootString, 0) != 0) {
        return false;
    }
    return candidateString[rootString.size()] == fs::path::preferred_separator;
}

fs::path resolvePathInsideRoot(const fs::path& installRoot, const std::string& relativePath, std::string& error)
{
    if (relativePath.empty()) {
        error = "Relative path cannot be empty";
        return {};
    }

    fs::path rel(relativePath);
    if (rel.is_absolute()) {
        error = "Manifest paths must be relative, not absolute";
        return {};
    }

    const fs::path combined = (installRoot / rel).lexically_normal();
    if (!isPathWithinRoot(combined, installRoot.lexically_normal())) {
        error = "Manifest path escapes install root: " + relativePath;
        return {};
    }

    return combined;
}

std::string runtimeDataDir(const std::string& appId);
std::string runtimeCacheDir(const std::string& appId);
std::string runtimeRunDir(const std::string& appId);

std::string defaultSocketLocation(const std::string& appId)
{
    return runtimeRunDir(appId) + "/app.sock";
}

bool isSafeSystemTokenRelativePath(const fs::path& relativePath)
{
    if (relativePath.empty() || relativePath.is_absolute()) {
        return false;
    }

    for (const auto& component : relativePath) {
        const auto value = component.string();
        if (value.empty() || value == "." || value == "..") {
            return false;
        }
    }

    return true;
}

std::optional<fs::path> resolveManifestTokenPath(const std::string& appId, const fs::path& installRoot, const std::string& token, std::string& error)
{
    if (token == "app://install") {
        return installRoot.lexically_normal();
    }
    if (token == "app://assets") {
        return (installRoot / "assets").lexically_normal();
    }
    if (token == "app://data") {
        return fs::path(runtimeDataDir(appId)).lexically_normal();
    }
    if (token == "app://cache") {
        return fs::path(runtimeCacheDir(appId)).lexically_normal();
    }
    if (token == "app://runtime") {
        return fs::path(runtimeRunDir(appId)).lexically_normal();
    }
    if (token.rfind("shared://readonly/", 0) == 0) {
        return fs::path("/usr/share/thecube") / token.substr(std::string("shared://readonly/").size());
    }
    if (token.rfind("system://", 0) == 0) {
        const auto relative = token.substr(std::string("system://").size());
        const fs::path relativePath(relative);
        if (!isSafeSystemTokenRelativePath(relativePath)) {
            error = "system:// paths must be normalized host-relative paths: " + token;
            return std::nullopt;
        }

        return (fs::path("/") / relativePath).lexically_normal();
    }
    return std::nullopt;
}

fs::path resolveTokenOrRelativePath(const std::string& appId, const fs::path& installRoot, const std::string& value, std::string& error)
{
    if (const auto tokenPath = resolveManifestTokenPath(appId, installRoot, value, error); tokenPath.has_value()) {
        return *tokenPath;
    }
    if (!error.empty()) {
        return {};
    }

    if (value.find("://") != std::string::npos) {
        error = "Unsupported path token: " + value;
        return {};
    }

    return resolvePathInsideRoot(installRoot, value, error);
}

std::optional<fs::path> findExecutableOnPath(const std::string& executableName)
{
    if (executableName.empty()) {
        return std::nullopt;
    }

    if (executableName.find(fs::path::preferred_separator) != std::string::npos) {
        fs::path candidate(executableName);
        std::error_code ec;
        if (candidate.is_relative()) {
            candidate = fs::absolute(candidate, ec);
            if (ec) {
                return std::nullopt;
            }
        }
        if (fs::exists(candidate, ec) && !ec) {
            return candidate.lexically_normal();
        }
        return std::nullopt;
    }

    if (const auto* rawPath = std::getenv("PATH"); rawPath != nullptr) {
        for (const auto& pathEntry : splitSearchPaths(rawPath)) {
            const fs::path candidate = fs::path(pathEntry) / executableName;
            std::error_code ec;
            if (fs::exists(candidate, ec) && !ec) {
                return fs::absolute(candidate, ec).lexically_normal();
            }
        }
    }

    for (const auto& fallback : { fs::path("/usr/bin") / executableName, fs::path("/usr/local/bin") / executableName, fs::path("/bin") / executableName }) {
        std::error_code ec;
        if (fs::exists(fallback, ec) && !ec) {
            return fallback.lexically_normal();
        }
    }

    return std::nullopt;
}

std::optional<fs::path> resolveDockerExecutablePath()
{
    const auto configured = Config::get("THECUBE_DOCKER_BIN", "");
    if (!configured.empty()) {
        return findExecutableOnPath(configured);
    }
    return findExecutableOnPath("docker");
}

std::optional<fs::path> resolveLauncherExecutablePath()
{
    const auto configured = Config::get("THECUBE_APP_LAUNCHER_BIN", "");
    if (!configured.empty()) {
        return findExecutableOnPath(configured);
    }

    std::error_code ec;
    const auto localLauncher = currentWorkingDirectoryOrFallback(fs::path(".")) / kDefaultLauncherBinaryName;
    if (fs::exists(localLauncher, ec) && !ec) {
        return fs::absolute(localLauncher, ec).lexically_normal();
    }

    if (fs::exists(kDefaultInstalledLauncherPath, ec) && !ec) {
        return fs::path(kDefaultInstalledLauncherPath);
    }

    return findExecutableOnPath(kDefaultLauncherBinaryName);
}

bool shouldUseUserSystemdManager()
{
    const auto configuredScope = trim(Config::get("THECUBE_SYSTEMD_SCOPE", ""));
    if (configuredScope == "user") {
        return true;
    }
    if (configuredScope == "system") {
        return false;
    }

#ifdef _WIN32
    return false;
#else
    return geteuid() != 0;
#endif
}

std::string systemctlCommandPrefix()
{
    return shouldUseUserSystemdManager() ? "systemctl --user" : "systemctl";
}

std::string systemdRunCommandPrefix()
{
    return shouldUseUserSystemdManager() ? "systemd-run --user" : "systemd-run";
}

std::optional<std::string> appIdFromUnitName(const std::string& unitName)
{
    constexpr std::string_view prefix = "thecube-app@";
    constexpr std::string_view suffix = ".service";

    if (unitName.size() <= prefix.size() + suffix.size()) {
        return std::nullopt;
    }
    if (unitName.rfind(prefix.data(), 0) != 0) {
        return std::nullopt;
    }
    if (unitName.compare(unitName.size() - suffix.size(), suffix.size(), suffix.data()) != 0) {
        return std::nullopt;
    }

    return unitName.substr(prefix.size(), unitName.size() - prefix.size() - suffix.size());
}

std::optional<RegistryRow> getRegistryRowByFilters(Database* db, const DB_NS::PredicateList& filters)
{
    const auto rows = db->selectData(
        kAppsTableName,
        { "app_id", "app_name", "manifest_path", "install_root", "enabled", "is_system_app", "autostart", "policy_compile_status", "policy_compile_error", "socket_location" },
        filters);

    if (rows.empty() || rows[0].size() < 10) {
        return std::nullopt;
    }

    RegistryRow row;
    row.appId = rows[0][0];
    row.appName = rows[0][1];
    row.manifestPath = rows[0][2];
    row.installRoot = rows[0][3];
    row.enabled = rows[0][4];
    row.isSystemApp = rows[0][5];
    row.autostart = rows[0][6];
    row.policyCompileStatus = rows[0][7];
    row.policyCompileError = rows[0][8];
    row.socketLocation = rows[0][9];
    return row;
}

ManifestParseResult loadManifestSummary(const fs::path& manifestPath)
{
    ManifestParseResult result;
    ManifestSummary summary;
    summary.manifestPath = fs::absolute(manifestPath).lexically_normal();
    summary.installRoot = summary.manifestPath.parent_path();

    std::ifstream input(summary.manifestPath);
    if (!input.is_open()) {
        result.error = "Unable to open manifest: " + summary.manifestPath.string();
        return result;
    }

    try {
        input >> summary.raw;
        if (!summary.raw.is_object()) {
            result.error = "Manifest root must be an object";
            return result;
        }

        summary.schemaVersion = summary.raw.value("schema_version", "");

        if (summary.raw.contains("app") && summary.raw["app"].is_object()) {
            const auto& app = summary.raw["app"];
            summary.appId = app.value("id", "");
            summary.appName = app.value("name", "");
            summary.appVersion = app.value("version", "");
            summary.appType = app.value("type", "");
            summary.workingDirectory = app.value("working_directory", std::string("."));
            summary.autostart = app.value("autostart", false);
            summary.systemApp = app.value("system_app", false);

            if (app.contains("args")) {
                if (!app["args"].is_array()) {
                    result.manifest = summary;
                    result.error = "app.args must be an array";
                    return result;
                }
                for (const auto& arg : app["args"]) {
                    if (!arg.is_string()) {
                        result.manifest = summary;
                        result.error = "app.args entries must be strings";
                        return result;
                    }
                    summary.args.push_back(arg.get<std::string>());
                }
            }
        }

        if (summary.raw.contains("runtime") && summary.raw["runtime"].is_object()) {
            const auto& runtime = summary.raw["runtime"];
            summary.runtimeType = runtime.value("type", "");
            summary.runtimeDistribution = runtime.value("distribution", "");
            summary.runtimeCompatibility = runtime.value("compatibility", "");
        }

        std::string postgresValidationError;
        if (!AppPostgresAccess::parseRequestedDatabases(summary.raw, summary.postgresqlDatabases, postgresValidationError)) {
            result.manifest = summary;
            result.error = postgresValidationError;
            return result;
        }
    } catch (const std::exception& e) {
        result.error = std::string("Failed to parse manifest JSON: ") + e.what();
        return result;
    }

    std::string validationError;
    if (summary.schemaVersion.empty()) {
        validationError = "Missing schema_version";
    } else if (summary.appId.empty()) {
        validationError = "Missing app.id";
    } else if (!std::regex_match(summary.appId, kAppIdPattern)) {
        validationError = "app.id does not match allowed pattern";
    } else if (summary.appName.empty()) {
        validationError = "Missing app.name";
    } else if (summary.appVersion.empty()) {
        validationError = "Missing app.version";
    } else if (summary.appType.empty()) {
        validationError = "Missing app.type";
    } else if (summary.runtimeType.empty()) {
        validationError = "Missing runtime.type";
    } else if (!summary.raw.contains("permissions") || !summary.raw["permissions"].is_object()) {
        validationError = "Missing permissions object";
    } else if (!summary.raw["permissions"].contains("filesystem") || !summary.raw["permissions"]["filesystem"].is_object()) {
        validationError = "Missing permissions.filesystem object";
    } else if (!summary.raw["permissions"]["filesystem"].contains("read_only") || !summary.raw["permissions"]["filesystem"].contains("read_write")) {
        validationError = "permissions.filesystem must contain read_only and read_write";
    } else if (summary.runtimeType == "native") {
        if (!summary.raw["runtime"].contains("native") || !summary.raw["runtime"]["native"].is_object()) {
            validationError = "Missing runtime.native object";
        } else if (!summary.raw["runtime"]["native"].contains("entrypoint") || !summary.raw["runtime"]["native"]["entrypoint"].is_string()) {
            validationError = "Missing runtime.native.entrypoint";
        }
    } else if (summary.runtimeType == "python") {
        if (!summary.raw["runtime"].contains("python") || !summary.raw["runtime"]["python"].is_object()) {
            validationError = "Missing runtime.python object";
        } else if (!summary.raw["runtime"]["python"].contains("entry_script") || !summary.raw["runtime"]["python"]["entry_script"].is_string()) {
            validationError = "Missing runtime.python.entry_script";
        } else if (summary.raw["runtime"]["python"].contains("package_name") && !summary.raw["runtime"]["python"]["package_name"].is_string()) {
            validationError = "runtime.python.package_name must be a string";
        }
    } else if (summary.runtimeType == "node") {
        if (!summary.raw["runtime"].contains("node") || !summary.raw["runtime"]["node"].is_object()) {
            validationError = "Missing runtime.node object";
        } else if (!summary.raw["runtime"]["node"].contains("entry_script") || !summary.raw["runtime"]["node"]["entry_script"].is_string()) {
            validationError = "Missing runtime.node.entry_script";
        }
    } else if (summary.runtimeType == "docker") {
        if (!summary.raw["runtime"].contains("docker") || !summary.raw["runtime"]["docker"].is_object()) {
            validationError = "Missing runtime.docker object";
        } else if (!summary.raw["runtime"]["docker"].contains("image") || !summary.raw["runtime"]["docker"]["image"].is_string()) {
            validationError = "Missing runtime.docker.image";
        }
    } else if (summary.runtimeType == "web-bundle") {
        if (!summary.raw["runtime"].contains("web") || !summary.raw["runtime"]["web"].is_object()) {
            validationError = "Missing runtime.web object";
        } else if (!summary.raw["runtime"]["web"].contains("entry") || !summary.raw["runtime"]["web"]["entry"].is_string()) {
            validationError = "Missing runtime.web.entry";
        }
    } else {
        validationError = "Unsupported runtime.type: " + summary.runtimeType;
    }

    result.manifest = summary;
    result.error = validationError;
    return result;
}

bool upsertRegistryEntry(const ManifestSummary& summary, const std::string& policyStatus, const std::string& policyError)
{
    auto* db = CubeDB::getDBManager()->getDatabase(kAppsDatabaseName);
    if (!db) {
        CubeLog::error("Apps DB unavailable while upserting manifest: " + summary.manifestPath.string());
        return false;
    }

    const DB_NS::PredicateList filters = { DB_NS::Predicate { "app_id", summary.appId } };
    const auto manifestPath = summary.manifestPath.string();
    const auto installRoot = summary.installRoot.string();
    const auto socketLocation = defaultSocketLocation(summary.appId);

    if (db->rowExists(kAppsTableName, filters)) {
        return db->updateData(
            kAppsTableName,
            { "app_name", "manifest_path", "install_root", "schema_version", "app_version", "app_type", "runtime_type", "runtime_distribution", "runtime_compatibility", "is_system_app", "autostart", "policy_compile_status", "policy_compile_error", "socket_location" },
            { summary.appName, manifestPath, installRoot, summary.schemaVersion, summary.appVersion, summary.appType, summary.runtimeType, summary.runtimeDistribution, summary.runtimeCompatibility, boolToDb(summary.systemApp), boolToDb(summary.autostart), policyStatus, policyError, socketLocation },
            filters);
    }

    return -1 < db->insertData(
                    kAppsTableName,
                    { "app_id", "app_name", "manifest_path", "install_root", "schema_version", "app_version", "app_type", "runtime_type", "runtime_distribution", "runtime_compatibility", "enabled", "is_system_app", "autostart", "policy_compile_status", "policy_compile_error", "socket_location" },
                    { summary.appId, summary.appName, manifestPath, installRoot, summary.schemaVersion, summary.appVersion, summary.appType, summary.runtimeType, summary.runtimeDistribution, summary.runtimeCompatibility, "1", boolToDb(summary.systemApp), boolToDb(summary.autostart), policyStatus, policyError, socketLocation });
}

void markManifestPathError(const fs::path& manifestPath, const std::string& error)
{
    auto* db = CubeDB::getDBManager()->getDatabase(kAppsDatabaseName);
    if (!db) {
        return;
    }

    const DB_NS::PredicateList filters = { DB_NS::Predicate { "manifest_path", fs::absolute(manifestPath).lexically_normal().string() } };
    if (!db->rowExists(kAppsTableName, filters)) {
        return;
    }

    db->updateData(
        kAppsTableName,
        { "policy_compile_status", "policy_compile_error" },
        { "error", error },
        filters);
}

void updateAppStatus(const std::string& appId, const std::string& status, const std::string& error)
{
    auto* db = CubeDB::getDBManager()->getDatabase(kAppsDatabaseName);
    if (!db) {
        return;
    }

    db->updateData(
        kAppsTableName,
        { "policy_compile_status", "policy_compile_error" },
        { status, error },
        { DB_NS::Predicate { "app_id", appId } });
}

void updateStartFailure(const std::string& appId, const std::string& reason)
{
    auto* db = CubeDB::getDBManager()->getDatabase(kAppsDatabaseName);
    if (!db) {
        return;
    }

    db->updateData(
        kAppsTableName,
        { "last_failure_reason" },
        { reason },
        { DB_NS::Predicate { "app_id", appId } });
}

void updateStartSuccess(const std::string& appId)
{
    auto* db = CubeDB::getDBManager()->getDatabase(kAppsDatabaseName);
    if (!db) {
        return;
    }

    db->updateData(
        kAppsTableName,
        { "last_started_at", "last_failure_reason", "last_exit_code" },
        { isoTimestampUtcNow(), "", "" },
        { DB_NS::Predicate { "app_id", appId } });
}

void updateStopSuccess(const std::string& appId)
{
    auto* db = CubeDB::getDBManager()->getDatabase(kAppsDatabaseName);
    if (!db) {
        return;
    }

    db->updateData(
        kAppsTableName,
        { "last_stopped_at", "last_failure_reason" },
        { isoTimestampUtcNow(), "" },
        { DB_NS::Predicate { "app_id", appId } });
}

std::optional<RegistryRow> getRegistryRowByAppId(const std::string& appId)
{
    auto* db = CubeDB::getDBManager()->getDatabase(kAppsDatabaseName);
    if (!db) {
        return std::nullopt;
    }
    return getRegistryRowByFilters(db, { DB_NS::Predicate { "app_id", appId } });
}

std::string buildUnitName(const std::string& appId)
{
    return "thecube-app@" + appId + ".service";
}

std::string runtimeDataDir(const std::string& appId)
{
    return (configuredBasePath("THECUBE_DATA_ROOT", kDefaultDataRoot, kDefaultLocalDataRoot) / appId).string();
}

std::string runtimeCacheDir(const std::string& appId)
{
    return (configuredBasePath("THECUBE_CACHE_ROOT", kDefaultCacheRoot, kDefaultLocalCacheRoot) / appId).string();
}

std::string runtimeRunDir(const std::string& appId)
{
    return (configuredBasePath("THECUBE_RUNTIME_ROOT", kDefaultRuntimeRoot, kDefaultLocalRuntimeRoot) / appId).string();
}

fs::path managedPythonVenvRoot(const std::string& appId)
{
    return fs::path(runtimeDataDir(appId)) / "venv";
}

std::optional<fs::path> findExistingPath(const std::vector<fs::path>& candidates);

std::vector<fs::path> pythonExecutableCandidates(const ManifestSummary& summary)
{
    return {
        managedPythonVenvRoot(summary.appId) / "bin/python",
        managedPythonVenvRoot(summary.appId) / "bin/python3",
        summary.installRoot / ".venv/bin/python",
        summary.installRoot / ".venv/bin/python3",
        summary.installRoot / "bin/python",
        summary.installRoot / "bin/python3"
    };
}

std::optional<fs::path> resolveHostPythonExecutable()
{
    const auto configured = Config::get("THECUBE_HOST_PYTHON_BIN", "");
    if (!configured.empty()) {
        return findExecutableOnPath(configured);
    }
    return findExecutableOnPath("python3");
}

std::string pythonPackageName(const ManifestSummary& summary)
{
    const auto& runtime = summary.raw["runtime"];
    if (runtime.contains("python") && runtime["python"].is_object()) {
        const auto& python = runtime["python"];
        if (python.contains("package_name") && python["package_name"].is_string()) {
            return python["package_name"].get<std::string>();
        }
    }
    return {};
}

fs::path pythonInstallStatePath(const std::string& appId)
{
    return fs::path(runtimeDataDir(appId)) / "python-install-state.json";
}

std::optional<PythonInstallState> readPythonInstallState(const std::string& appId)
{
    const auto path = pythonInstallStatePath(appId);
    std::ifstream input(path);
    if (!input.is_open()) {
        return std::nullopt;
    }

    try {
        nlohmann::json json;
        input >> json;
        if (!json.is_object()) {
            return std::nullopt;
        }

        PythonInstallState state;
        state.appVersion = json.value("app_version", "");
        state.packageName = json.value("package_name", "");
        state.venvRoot = json.value("venv_root", "");
        if (state.appVersion.empty() || state.venvRoot.empty()) {
            return std::nullopt;
        }
        return state;
    } catch (...) {
        return std::nullopt;
    }
}

bool writePythonInstallState(const ManifestSummary& summary, const fs::path& venvRoot, const std::string& packageName, std::string& error)
{
    const auto path = pythonInstallStatePath(summary.appId);
    std::ofstream output(path);
    if (!output.is_open()) {
        error = "Failed to write Python install state file: " + path.string();
        return false;
    }

    output << nlohmann::json {
        { "app_version", summary.appVersion },
        { "package_name", packageName },
        { "venv_root", venvRoot.string() }
    }.dump(2);
    return true;
}

bool runShellCommand(const std::string& command, const std::string& description, std::string& error)
{
    CubeLog::info("AppsManager: " + description);
    const int rc = extractCommandExitCode(std::system(command.c_str()));
    if (rc == 0) {
        return true;
    }

    error = description + " failed (rc=" + std::to_string(rc) + ")";
    return false;
}

bool ensurePythonRuntimeInstalled(const ManifestSummary& summary, std::string& error)
{
    if (summary.runtimeType != "python") {
        return true;
    }
    if (summary.runtimeDistribution != "venv") {
        error = "Python apps must use runtime.distribution=venv";
        return false;
    }

    const auto packageName = pythonPackageName(summary);
    if (packageName.empty()) {
        if (findExistingPath(pythonExecutableCandidates(summary)).has_value()) {
            CubeLog::info("AppsManager: using existing Python runtime for script app " + summary.appId);
            return true;
        }
    }

    const auto venvRoot = managedPythonVenvRoot(summary.appId);
    const auto pythonBinary = venvRoot / "bin/python3";
    const auto pipBinary = venvRoot / "bin/pip";
    const auto existingState = readPythonInstallState(summary.appId);
    if (existingState.has_value()
        && existingState->appVersion == summary.appVersion
        && existingState->packageName == packageName
        && existingState->venvRoot == venvRoot.string()
        && fs::exists(pythonBinary)
        && fs::exists(pipBinary)) {
        CubeLog::info("AppsManager: Python runtime already installed for " + summary.appId);
        return true;
    }

    const auto hostPython = resolveHostPythonExecutable();
    if (!hostPython.has_value()) {
        error = "Host python3 executable was not found. Set THECUBE_HOST_PYTHON_BIN if it is installed in a non-standard location.";
        return false;
    }

    std::error_code ec;
    fs::create_directories(venvRoot.parent_path(), ec);
    if (ec) {
        error = "Failed to create Python venv parent directory: " + ec.message();
        return false;
    }

    if (!fs::exists(pythonBinary)) {
        const auto createVenvCommand = shellQuote(hostPython->string()) + " -m venv " + shellQuote(venvRoot.string());
        if (!runShellCommand(createVenvCommand, "creating Python venv for " + summary.appId, error)) {
            return false;
        }
    }

    if (!packageName.empty()) {
        if (!fs::exists(pipBinary)) {
            error = "Python venv pip executable does not exist after bootstrap: " + pipBinary.string();
            return false;
        }

        const auto upgradeCommand = shellQuote(pipBinary.string()) + " install --upgrade pip setuptools wheel";
        if (!runShellCommand(upgradeCommand, "upgrading pip/setuptools/wheel for " + summary.appId, error)) {
            return false;
        }

        const auto installCommand = shellQuote(pipBinary.string()) + " install " + shellQuote(packageName);
        if (!runShellCommand(installCommand, "installing Python package " + packageName + " for " + summary.appId, error)) {
            return false;
        }
    }

    if (!writePythonInstallState(summary, venvRoot, packageName, error)) {
        return false;
    }

    if (packageName.empty()) {
        CubeLog::info("AppsManager: Python runtime is ready for script app " + summary.appId);
    } else {
        CubeLog::info("AppsManager: Python runtime is ready for " + summary.appId + " using package " + packageName);
    }
    return true;
}

std::optional<fs::path> findExistingPath(const std::vector<fs::path>& candidates)
{
    std::error_code ec;
    for (const auto& candidate : candidates) {
        if (fs::exists(candidate, ec) && !ec) {
            return candidate;
        }
    }
    return std::nullopt;
}

bool appendFilesystemPaths(
    const std::string& appId,
    const fs::path& installRoot,
    const nlohmann::json& paths,
    nlohmann::json& out,
    std::string& error)
{
    if (!paths.is_array()) {
        error = "Filesystem permission entries must be arrays";
        return false;
    }

    for (const auto& entry : paths) {
        if (!entry.is_string()) {
            error = "Filesystem permission entries must be strings";
            return false;
        }

        const auto token = entry.get<std::string>();
        std::string pathError;
        const auto resolved = resolveTokenOrRelativePath(appId, installRoot, token, pathError);
        if (!pathError.empty()) {
            error = pathError;
            return false;
        }

        out.push_back(resolved.string());
    }

    return true;
}

bool compileLaunchPolicy(const ManifestSummary& summary, nlohmann::json& policyOut, std::string& error)
{
    std::string workingDirectoryError;
    const auto workingDirectory = resolvePathInsideRoot(summary.installRoot, summary.workingDirectory, workingDirectoryError);
    if (!workingDirectoryError.empty()) {
        error = workingDirectoryError;
        return false;
    }

    std::string entryField;
    std::string resolvedExecutable;
    std::string launchTarget;
    std::vector<std::string> argv;

    const auto& runtime = summary.raw["runtime"];
    if (summary.runtimeType == "native") {
        entryField = runtime["native"]["entrypoint"].get<std::string>();
        std::string pathError;
        const auto entrypoint = resolvePathInsideRoot(summary.installRoot, entryField, pathError);
        if (!pathError.empty()) {
            error = pathError;
            return false;
        }
        if (!fs::exists(entrypoint)) {
            error = "Native entrypoint does not exist: " + entrypoint.string();
            return false;
        }

        resolvedExecutable = entrypoint.string();
        launchTarget = entrypoint.string();
        argv.push_back(resolvedExecutable);
    } else if (summary.runtimeType == "python") {
        entryField = runtime["python"]["entry_script"].get<std::string>();
        std::string pathError;
        const auto entryScript = resolvePathInsideRoot(summary.installRoot, entryField, pathError);
        if (!pathError.empty()) {
            error = pathError;
            return false;
        }
        if (!fs::exists(entryScript)) {
            error = "Python entry script does not exist: " + entryScript.string();
            return false;
        }

        if (summary.runtimeDistribution == "venv") {
            const auto pythonPath = findExistingPath(pythonExecutableCandidates(summary));
            if (!pythonPath.has_value()) {
                error = "Python venv executable does not exist for app " + summary.appId;
                return false;
            }
            resolvedExecutable = pythonPath->lexically_normal().string();
        } else {
            if (summary.runtimeCompatibility.empty()) {
                error = "Python runtime.compatibility is required for platform runtimes";
                return false;
            }
            const fs::path runtimePath("/opt/thecube/runtimes/python/" + summary.runtimeCompatibility + "/bin/python3");
            if (!fs::exists(runtimePath)) {
                error = "Platform Python runtime does not exist: " + runtimePath.string();
                return false;
            }
            resolvedExecutable = runtimePath.string();
        }

        launchTarget = entryScript.string();
        argv.push_back(resolvedExecutable);
        argv.push_back(launchTarget);
    } else if (summary.runtimeType == "node") {
        entryField = runtime["node"]["entry_script"].get<std::string>();
        std::string pathError;
        const auto entryScript = resolvePathInsideRoot(summary.installRoot, entryField, pathError);
        if (!pathError.empty()) {
            error = pathError;
            return false;
        }
        if (!fs::exists(entryScript)) {
            error = "Node entry script does not exist: " + entryScript.string();
            return false;
        }
        if (summary.runtimeCompatibility.empty()) {
            error = "Node runtime.compatibility is required";
            return false;
        }

        const fs::path runtimePath("/opt/thecube/runtimes/node/" + summary.runtimeCompatibility + "/bin/node");
        if (!fs::exists(runtimePath)) {
            error = "Platform Node runtime does not exist: " + runtimePath.string();
            return false;
        }

        resolvedExecutable = runtimePath.string();
        launchTarget = entryScript.string();
        argv.push_back(resolvedExecutable);
        argv.push_back(launchTarget);
    } else if (summary.runtimeType == "docker") {
        const auto dockerExecutable = resolveDockerExecutablePath();
        if (!dockerExecutable.has_value()) {
            error = "Docker executable was not found. Set THECUBE_DOCKER_BIN if it is installed in a non-standard location.";
            return false;
        }

        const auto& docker = runtime["docker"];
        const auto image = docker.value("image", std::string());
        if (image.empty()) {
            error = "runtime.docker.image is required";
            return false;
        }

        const auto containerName = docker.value("container_name", summary.appId);

        resolvedExecutable = dockerExecutable->string();
        launchTarget = image;
        argv = {
            resolvedExecutable,
            "run",
            "--rm",
            "--name",
            containerName
        };

        if (docker.contains("published_ports")) {
            if (!docker["published_ports"].is_array()) {
                error = "runtime.docker.published_ports must be an array";
                return false;
            }
            for (const auto& entry : docker["published_ports"]) {
                if (!entry.is_string()) {
                    error = "runtime.docker.published_ports entries must be strings";
                    return false;
                }
                argv.push_back("-p");
                argv.push_back(entry.get<std::string>());
            }
        }

        if (docker.contains("environment")) {
            if (!docker["environment"].is_object()) {
                error = "runtime.docker.environment must be an object";
                return false;
            }
            std::map<std::string, std::string> dockerEnvironment;
            for (auto it = docker["environment"].begin(); it != docker["environment"].end(); ++it) {
                if (!it.value().is_string()) {
                    error = "runtime.docker.environment values must be strings";
                    return false;
                }
                dockerEnvironment[it.key()] = it.value().get<std::string>();
            }

            if (summary.appId == Config::get("APPS_POSTGRES_APP_ID", kDefaultManagedPostgresAppId)) {
                dockerEnvironment.erase("POSTGRES_HOST_AUTH_METHOD");
                dockerEnvironment["POSTGRES_USER"] = Config::get("APPS_POSTGRES_ADMIN_USER", "postgres");
                dockerEnvironment["POSTGRES_PASSWORD"] = Config::get("APPS_POSTGRES_ADMIN_PASSWORD", "thecube-postgres-admin");
                dockerEnvironment["POSTGRES_DB"] = Config::get("APPS_POSTGRES_ADMIN_DB", "postgres");
            }

            for (const auto& [key, value] : dockerEnvironment) {
                argv.push_back("-e");
                argv.push_back(key + "=" + value);
            }
        }

        if (docker.contains("volumes")) {
            if (!docker["volumes"].is_array()) {
                error = "runtime.docker.volumes must be an array";
                return false;
            }
            for (const auto& volume : docker["volumes"]) {
                if (!volume.is_object()) {
                    error = "runtime.docker.volumes entries must be objects";
                    return false;
                }
                if (!volume.contains("host") || !volume["host"].is_string()) {
                    error = "runtime.docker.volumes entries must contain string host values";
                    return false;
                }
                if (!volume.contains("container") || !volume["container"].is_string()) {
                    error = "runtime.docker.volumes entries must contain string container values";
                    return false;
                }

                std::string hostPathError;
                const auto hostPath = resolveTokenOrRelativePath(summary.appId, summary.installRoot, volume["host"].get<std::string>(), hostPathError);
                if (!hostPathError.empty()) {
                    error = hostPathError;
                    return false;
                }

                const auto containerPath = volume["container"].get<std::string>();
                if (containerPath.empty() || containerPath.front() != '/') {
                    error = "runtime.docker.volumes container paths must be absolute";
                    return false;
                }

                argv.push_back("-v");
                argv.push_back(hostPath.string() + ":" + containerPath);
            }
        }

        argv.push_back(image);
    } else if (summary.runtimeType == "web-bundle") {
        error = "web-bundle startup is not implemented in AppsManager v1";
        return false;
    } else {
        error = "Unsupported runtime.type: " + summary.runtimeType;
        return false;
    }

    for (const auto& arg : summary.args) {
        argv.push_back(arg);
    }

    nlohmann::json readOnly = nlohmann::json::array();
    nlohmann::json readWrite = nlohmann::json::array();
    std::string fsError;
    if (!appendFilesystemPaths(summary.appId, summary.installRoot, summary.raw["permissions"]["filesystem"]["read_only"], readOnly, fsError)) {
        error = fsError;
        return false;
    }
    if (!appendFilesystemPaths(summary.appId, summary.installRoot, summary.raw["permissions"]["filesystem"]["read_write"], readWrite, fsError)) {
        error = fsError;
        return false;
    }

    nlohmann::json allowInherit = nlohmann::json::array();
    if (summary.raw.contains("environment") && summary.raw["environment"].is_object() && summary.raw["environment"].contains("allow_inherit")) {
        if (!summary.raw["environment"]["allow_inherit"].is_array()) {
            error = "environment.allow_inherit must be an array";
            return false;
        }
        for (const auto& value : summary.raw["environment"]["allow_inherit"]) {
            if (!value.is_string()) {
                error = "environment.allow_inherit entries must be strings";
                return false;
            }
            allowInherit.push_back(value.get<std::string>());
        }
    }

    nlohmann::json environmentSet = {
        { "THECUBE_APP_ID", summary.appId },
        { "THECUBE_DATA_DIR", runtimeDataDir(summary.appId) },
        { "THECUBE_CACHE_DIR", runtimeCacheDir(summary.appId) },
        { "THECUBE_RUNTIME_DIR", runtimeRunDir(summary.appId) },
        { "THECUBE_CORE_SOCKET", kDefaultCoreSocket }
    };
    if (!AppPostgresAccess::appendLaunchEnvironment(summary.appId, summary.postgresqlDatabases, environmentSet, error)) {
        return false;
    }
    if (summary.raw.contains("environment") && summary.raw["environment"].is_object() && summary.raw["environment"].contains("set")) {
        if (!summary.raw["environment"]["set"].is_object()) {
            error = "environment.set must be an object";
            return false;
        }
        for (auto it = summary.raw["environment"]["set"].begin(); it != summary.raw["environment"]["set"].end(); ++it) {
            if (!it.value().is_string()) {
                error = "environment.set values must be strings";
                return false;
            }
            environmentSet[it.key()] = it.value().get<std::string>();
        }
    }

    std::ifstream manifestFile(summary.manifestPath);
    std::stringstream buffer;
    buffer << manifestFile.rdbuf();

    policyOut = {
        { "policy_version", "1.0" },
        { "launch_id", summary.appId + "-" + isoTimestampUtcNow() },
        { "generated_at", isoTimestampUtcNow() },
        { "app", {
              { "id", summary.appId },
              { "name", summary.appName },
              { "type", summary.appType },
              { "version", summary.appVersion },
              { "install_root", summary.installRoot.string() },
              { "argv", argv },
              { "working_directory", workingDirectory.string() }
          } },
        { "runtime", {
              { "type", summary.runtimeType },
              { "distribution", summary.runtimeDistribution },
              { "compatibility", summary.runtimeCompatibility },
              { "resolved_executable", resolvedExecutable },
              { "launch_target", launchTarget }
          } },
        { "environment", {
              { "clear_inherited", true },
              { "allow_inherit", allowInherit },
              { "set", environmentSet }
          } },
        { "filesystem", {
              { "landlock", {
                    { "read_only", readOnly },
                    { "read_write", readWrite },
                    { "create", nlohmann::json::array({ runtimeDataDir(summary.appId), runtimeCacheDir(summary.appId), runtimeRunDir(summary.appId) }) }
                } }
          } },
        { "platform", {
              { "allowed_permissions", summary.raw["permissions"].value("platform", nlohmann::json::array()) },
              { "ipc", {
                    { "core_socket", kDefaultCoreSocket },
                    { "app_socket", defaultSocketLocation(summary.appId) }
                } }
          } },
        { "network", summary.raw["permissions"].value("network", nlohmann::json::object()) },
        { "integrity", {
              { "manifest_digest_sha256", sha256(buffer.str()) }
          } }
    };

    if (summary.runtimeType == "docker") {
        policyOut["runtime"]["docker"] = summary.raw["runtime"]["docker"];
    }

    return true;
}

bool writeLaunchPolicy(const std::string& appId, const nlohmann::json& policy, std::string& error)
{
    fs::path launchRoot = configuredBasePath("THECUBE_LAUNCH_ROOT", kDefaultLaunchRoot, kDefaultLocalLaunchRoot);

    const auto appLaunchDir = launchRoot / appId;
    std::error_code ec;
    fs::create_directories(appLaunchDir, ec);
    if (ec) {
        error = "Failed to create launch policy directory: " + ec.message();
        return false;
    }

    std::ofstream output(appLaunchDir / "launch-policy.json");
    if (!output.is_open()) {
        error = "Failed to write launch policy file";
        return false;
    }

    output << policy.dump(2);
    return true;
}

bool ensureAppRuntimeDirectoriesExist(const std::string& appId, std::string& error)
{
    for (const auto& dir : { fs::path(runtimeDataDir(appId)), fs::path(runtimeCacheDir(appId)), fs::path(runtimeRunDir(appId)) }) {
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec) {
            error = "Failed to create runtime directory " + dir.string() + ": " + ec.message();
            return false;
        }
    }
    return true;
}

int runCommand(const std::string& command)
{
    return std::system(command.c_str());
}

std::string captureCommandOutput(const std::string& command)
{
#ifdef _WIN32
    (void)command;
    return {};
#else
    std::array<char, 256> buffer {};
    std::string output;
    // TODO: popen() on a composed shell command is risky because quoting/injection and cleanup are easy to get wrong here; switch to an argument-vector subprocess API with RAII-managed pipes.
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return {};
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }

    pclose(pipe);
    return output;
#endif
}

struct UnitInspection {
    bool loaded = false;
    bool transient = false;
    bool failed = false;
    std::string execStart;
    std::string environment;
};

std::string systemctlShowValue(const std::string& output, const std::string& key)
{
    std::istringstream stream(output);
    std::string line;
    const std::string prefix = key + "=";
    while (std::getline(stream, line)) {
        if (line.rfind(prefix, 0) == 0) {
            return line.substr(prefix.size());
        }
    }
    return {};
}

UnitInspection inspectUnit(const std::string& unitName)
{
    UnitInspection inspection;
    const auto command = systemctlCommandPrefix() + " show "
        + shellQuote(unitName)
        + " -p FragmentPath -p UnitFileState -p LoadState -p ActiveState -p SubState -p ExecStart -p Environment 2>/dev/null";
    const auto output = captureCommandOutput(command);
    if (output.empty()) {
        return inspection;
    }

    const auto fragmentPath = systemctlShowValue(output, "FragmentPath");
    const auto unitFileState = systemctlShowValue(output, "UnitFileState");
    const auto loadState = systemctlShowValue(output, "LoadState");
    const auto activeState = systemctlShowValue(output, "ActiveState");
    const auto subState = systemctlShowValue(output, "SubState");
    inspection.execStart = systemctlShowValue(output, "ExecStart");
    inspection.environment = systemctlShowValue(output, "Environment");
    inspection.loaded = loadState == "loaded";
    inspection.transient = unitFileState == "transient" || fragmentPath.find("/transient/") != std::string::npos;
    inspection.failed = activeState == "failed" || subState == "failed";
    return inspection;
}

bool transientUnitNeedsReplacement(
    const UnitInspection& inspection,
    const std::string& launcherExecutable,
    const std::string& appId,
    const fs::path& launchRoot)
{
    if (!inspection.loaded || !inspection.transient) {
        return false;
    }

    if (inspection.failed) {
        return true;
    }

    if (inspection.execStart.find(launcherExecutable) == std::string::npos) {
        return true;
    }

    if (inspection.execStart.find(appId) == std::string::npos) {
        return true;
    }

    const auto launchRootSetting = "THECUBE_LAUNCH_ROOT=" + launchRoot.string();
    if (inspection.environment.find(launchRootSetting) == std::string::npos) {
        return true;
    }

    return false;
}

void clearTransientUnitState(const std::string& unitName)
{
    const auto stopCommand = systemctlCommandPrefix() + " stop " + shellQuote(unitName) + " >/dev/null 2>&1";
    runCommand(stopCommand);

    const auto resetFailedCommand = systemctlCommandPrefix() + " reset-failed " + shellQuote(unitName) + " >/dev/null 2>&1";
    runCommand(resetFailedCommand);
}

std::string buildLauncherRunCommand(
    const std::string& unitName,
    const std::string& appId,
    const fs::path& launcherExecutable,
    const fs::path& launchRoot)
{
    std::string command = systemdRunCommandPrefix()
        + " --quiet";
    command += " --unit " + shellQuote(unitName)
        + " --service-type=exec"
        + " --property=" + shellQuote("Restart=on-failure")
        + " --property=" + shellQuote("RestartSec=1")
        + " --setenv=" + shellQuote("THECUBE_LAUNCH_ROOT=" + launchRoot.string())
        + " " + shellQuote(launcherExecutable.string())
        + " " + shellQuote(appId)
        + " >/dev/null 2>&1";
    return command;
}
} // namespace

bool SystemdAppRuntimeController::startUnit(const std::string& unitName, std::string* errorOut)
{
    const auto appId = appIdFromUnitName(unitName);
    const auto launcherExecutable = resolveLauncherExecutablePath();
    const auto launchRoot = configuredBasePath("THECUBE_LAUNCH_ROOT", kDefaultLaunchRoot, kDefaultLocalLaunchRoot);

    if (appId.has_value() && launcherExecutable.has_value()) {
        const auto inspection = inspectUnit(unitName);
        if (transientUnitNeedsReplacement(inspection, launcherExecutable->string(), *appId, launchRoot)) {
            clearTransientUnitState(unitName);
            const auto replaceCommand = buildLauncherRunCommand(unitName, *appId, *launcherExecutable, launchRoot);
            const int replaceRc = extractCommandExitCode(runCommand(replaceCommand));
            if (replaceRc == 0) {
                return true;
            }
            if (errorOut != nullptr) {
                *errorOut = "transient replacement failed for " + unitName + " (rc=" + std::to_string(replaceRc) + ")";
            }
        }
    }

    const auto command = systemctlCommandPrefix() + " start " + shellQuote(unitName) + " >/dev/null 2>&1";
    const int rc = extractCommandExitCode(runCommand(command));
    if (rc == 0) {
        return true;
    }

    if (!appId.has_value()) {
        if (errorOut != nullptr) {
            *errorOut = "systemctl start failed and unit name could not be mapped to an app id: " + unitName;
        }
        return false;
    }

    if (!launcherExecutable.has_value()) {
        if (errorOut != nullptr) {
            *errorOut = "systemctl start failed for " + unitName + " and CubeAppLauncher was not found";
        }
        return false;
    }

    const auto fallbackCommand = buildLauncherRunCommand(unitName, *appId, *launcherExecutable, launchRoot);
    const int fallbackRc = extractCommandExitCode(runCommand(fallbackCommand));
    if (fallbackRc == 0) {
        return true;
    }

    if (errorOut != nullptr) {
        *errorOut = "systemctl start failed for " + unitName + " (rc=" + std::to_string(rc)
            + "); transient fallback failed (rc=" + std::to_string(fallbackRc) + ")";
    }
    return false;
}

bool SystemdAppRuntimeController::stopUnit(const std::string& unitName, std::string* errorOut)
{
    const auto command = systemctlCommandPrefix() + " stop " + shellQuote(unitName) + " >/dev/null 2>&1";
    const int rc = extractCommandExitCode(runCommand(command));
    if (rc == 0) {
        return true;
    }

    if (errorOut != nullptr) {
        *errorOut = "systemctl stop failed for " + unitName + " (rc=" + std::to_string(rc) + ")";
    }
    return false;
}

bool SystemdAppRuntimeController::isUnitActive(const std::string& unitName, std::string* errorOut) const
{
    const auto command = systemctlCommandPrefix() + " is-active --quiet " + shellQuote(unitName) + " >/dev/null 2>&1";
    const int rc = extractCommandExitCode(runCommand(command));
    if (rc == 0) {
        return true;
    }

    if (errorOut != nullptr) {
        *errorOut = "systemctl is-active returned non-zero for " + unitName + " (rc=" + std::to_string(rc) + ")";
    }
    return false;
}

AppsManager::AppsManager()
    : AppsManager(std::make_shared<SystemdAppRuntimeController>())
{
}

AppsManager::AppsManager(std::shared_ptr<AppRuntimeController> runtimeController)
    : runtimeController(std::move(runtimeController))
{
}

bool AppsManager::waitForDatabaseManager() const
{
    for (int attempt = 0; attempt < 500; ++attempt) {
        try {
            auto dbManager = CubeDB::getDBManager();
            if (dbManager != nullptr && dbManager->isDatabaseManagerReady()) {
                return true;
            }
        } catch (...) {
        }
        genericSleep(10);
    }

    CubeLog::error("AppsManager: timed out waiting for DB manager");
    return false;
}

bool AppsManager::syncRegistry()
{
    auto* db = CubeDB::getDBManager()->getDatabase(kAppsDatabaseName);
    if (!db) {
        CubeLog::error("AppsManager: apps DB unavailable during sync");
        return false;
    }

    const auto manifestPaths = discoverManifestPaths();
    CubeLog::info("AppsManager: manifest discovery found " + std::to_string(manifestPaths.size()) + " manifest(s)");
    std::unordered_set<std::string> seenManifestPaths;
    std::vector<ManifestSummary> validSummaries;
    bool allSucceeded = true;

    for (const auto& manifestPath : manifestPaths) {
        const auto normalizedManifest = fs::absolute(manifestPath).lexically_normal().string();
        seenManifestPaths.insert(normalizedManifest);
        CubeLog::info("AppsManager: syncing manifest " + normalizedManifest);

        auto parsed = loadManifestSummary(manifestPath);
        if (!parsed.manifest.has_value()) {
            CubeLog::error("AppsManager: manifest sync failed for " + manifestPath.string() + ": " + parsed.error);
            markManifestPathError(manifestPath, parsed.error);
            allSucceeded = false;
            continue;
        }

        const auto& summary = *parsed.manifest;
        const auto status = parsed.error.empty() ? "pending" : "error";
        if (!upsertRegistryEntry(summary, status, parsed.error)) {
            CubeLog::error("AppsManager: failed to upsert app manifest for " + summary.appId + ": " + db->getLastError());
            allSucceeded = false;
            continue;
        }

        CubeLog::info("AppsManager: synced app " + summary.appId + " from " + summary.manifestPath.string());

        if (!parsed.error.empty()) {
            CubeLog::warning("AppsManager: manifest validation error for " + summary.appId + ": " + parsed.error);
            allSucceeded = false;
        } else {
            validSummaries.push_back(summary);
        }
    }

    const auto rows = db->selectData(kAppsTableName, { "app_id", "manifest_path" });
    for (const auto& row : rows) {
        if (row.size() < 2) {
            continue;
        }
        if (seenManifestPaths.find(row[1]) == seenManifestPaths.end()) {
            CubeLog::warning("AppsManager: removing stale app registry row for " + row[0] + " because manifest is gone");
            db->deleteData(kAppsTableName, { DB_NS::Predicate { "app_id", row[0] } });
        }
    }

    const auto ensureSystemAppRunning = [this](const std::string& systemAppId, std::string& reason) {
        if (this->isAppRunning(systemAppId)) {
            return true;
        }
        if (this->startApp(systemAppId)) {
            return true;
        }
        reason = "Failed to start PostgreSQL system app: " + systemAppId;
        return false;
    };

    for (const auto& summary : validSummaries) {
        std::string postgresError;
        if (!AppPostgresAccess::provisionForApp(summary.appId, summary.raw, ensureSystemAppRunning, postgresError)) {
            CubeLog::error("AppsManager: PostgreSQL provisioning failed for " + summary.appId + ": " + postgresError);
            updateAppStatus(summary.appId, "error", postgresError);
            allSucceeded = false;
        }
    }

    return allSucceeded;
}

bool AppsManager::launchStartupApps()
{
    auto* db = CubeDB::getDBManager()->getDatabase(kAppsDatabaseName);
    if (!db) {
        CubeLog::error("AppsManager: apps DB unavailable during startup launch");
        return false;
    }

    bool allSucceeded = true;
    const auto rows = db->selectData(
        kAppsTableName,
        { "app_id", "enabled", "is_system_app", "autostart", "policy_compile_status" });

    for (const auto& row : rows) {
        if (row.size() < 5) {
            continue;
        }

        const bool enabled = dbToBool(row[1]);
        const bool systemApp = dbToBool(row[2]);
        const bool autostart = dbToBool(row[3]);
        const bool manifestErrored = row[4] == "error";
        CubeLog::info(
            "AppsManager: startup evaluation for " + row[0]
            + " enabled=" + row[1]
            + " system_app=" + row[2]
            + " autostart=" + row[3]
            + " policy_status=" + row[4]);
        if (!enabled || manifestErrored || (!systemApp && !autostart)) {
            continue;
        }

        if (!this->startApp(row[0])) {
            allSucceeded = false;
        }
    }

    return allSucceeded;
}

bool AppsManager::initialize()
{
    if (!waitForDatabaseManager()) {
        return false;
    }

    const bool syncSucceeded = this->syncRegistry();
    const bool startupSucceeded = this->launchStartupApps();
    if (!startupSucceeded) {
        CubeLog::warning("AppsManager: one or more startup apps failed to launch. CORE startup will continue.");
    }

    this->initialized = true;
    return syncSucceeded;
}

bool AppsManager::startApp(const std::string& appID)
{
    if (!waitForDatabaseManager()) {
        return false;
    }
    if (!runtimeController) {
        CubeLog::error("AppsManager: no runtime controller is configured");
        return false;
    }

    auto row = getRegistryRowByAppId(appID);
    if (!row.has_value()) {
        CubeLog::error("AppsManager: cannot start unknown app: " + appID);
        return false;
    }
    if (!dbToBool(row->enabled)) {
        CubeLog::warning("AppsManager: app is disabled and will not be started: " + appID);
        return false;
    }

    auto parsed = loadManifestSummary(row->manifestPath);
    if (!parsed.manifest.has_value()) {
        updateAppStatus(appID, "error", parsed.error);
        updateStartFailure(appID, parsed.error);
        CubeLog::error("AppsManager: failed to load manifest for " + appID + ": " + parsed.error);
        return false;
    }
    if (!parsed.error.empty()) {
        updateAppStatus(appID, "error", parsed.error);
        updateStartFailure(appID, parsed.error);
        CubeLog::error("AppsManager: manifest validation failed for " + appID + ": " + parsed.error);
        return false;
    }
    if (parsed.manifest->appId != appID) {
        const auto mismatch = "Manifest app.id does not match registry app_id";
        updateAppStatus(appID, "error", mismatch);
        updateStartFailure(appID, mismatch);
        CubeLog::error(std::string("AppsManager: ") + mismatch + " for " + appID);
        return false;
    }

    std::string runtimeDirectoryError;
    if (!ensureAppRuntimeDirectoriesExist(appID, runtimeDirectoryError)) {
        updateAppStatus(appID, "error", runtimeDirectoryError);
        updateStartFailure(appID, runtimeDirectoryError);
        CubeLog::error("AppsManager: failed to prepare runtime directories for " + appID + ": " + runtimeDirectoryError);
        return false;
    }

    std::string pythonInstallError;
    if (!ensurePythonRuntimeInstalled(*parsed.manifest, pythonInstallError)) {
        updateAppStatus(appID, "error", pythonInstallError);
        updateStartFailure(appID, pythonInstallError);
        CubeLog::error("AppsManager: failed to prepare Python runtime for " + appID + ": " + pythonInstallError);
        return false;
    }

    const auto ensureSystemAppRunning = [this](const std::string& systemAppId, std::string& reason) {
        if (this->isAppRunning(systemAppId)) {
            return true;
        }
        if (this->startApp(systemAppId)) {
            return true;
        }
        reason = "Failed to start PostgreSQL system app: " + systemAppId;
        return false;
    };
    std::string postgresProvisionError;
    if (!AppPostgresAccess::provisionForApp(appID, parsed.manifest->raw, ensureSystemAppRunning, postgresProvisionError)) {
        updateAppStatus(appID, "error", postgresProvisionError);
        updateStartFailure(appID, postgresProvisionError);
        CubeLog::error("AppsManager: failed to provision PostgreSQL access for " + appID + ": " + postgresProvisionError);
        return false;
    }

    nlohmann::json launchPolicy;
    std::string compileError;
    if (!compileLaunchPolicy(*parsed.manifest, launchPolicy, compileError)) {
        updateAppStatus(appID, "error", compileError);
        updateStartFailure(appID, compileError);
        CubeLog::error("AppsManager: launch policy compilation failed for " + appID + ": " + compileError);
        return false;
    }

    std::string writeError;
    if (!writeLaunchPolicy(appID, launchPolicy, writeError)) {
        updateAppStatus(appID, "error", writeError);
        updateStartFailure(appID, writeError);
        CubeLog::error("AppsManager: failed to write launch policy for " + appID + ": " + writeError);
        return false;
    }

    updateAppStatus(appID, "compiled", "");

    std::string controllerError;
    if (!runtimeController->startUnit(buildUnitName(appID), &controllerError)) {
        updateStartFailure(appID, controllerError);
        CubeLog::warning("AppsManager: startup request failed for " + appID + ": " + controllerError);
        return false;
    }

    updateStartSuccess(appID);
    CubeLog::info("AppsManager: requested start for app " + appID);
    return true;
}

bool AppsManager::stopApp(const std::string& appID)
{
    if (!waitForDatabaseManager()) {
        return false;
    }
    if (!runtimeController) {
        CubeLog::error("AppsManager: no runtime controller is configured");
        return false;
    }

    auto row = getRegistryRowByAppId(appID);
    if (!row.has_value()) {
        CubeLog::error("AppsManager: cannot stop unknown app: " + appID);
        return false;
    }

    std::string controllerError;
    if (!runtimeController->stopUnit(buildUnitName(appID), &controllerError)) {
        updateStartFailure(appID, controllerError);
        CubeLog::warning("AppsManager: stop request failed for " + appID + ": " + controllerError);
        return false;
    }

    updateStopSuccess(appID);
    CubeLog::info("AppsManager: requested stop for app " + appID);
    return true;
}

bool AppsManager::isAppRunning(const std::string& appID) const
{
    if (!waitForDatabaseManager() || !runtimeController) {
        return false;
    }

    auto row = getRegistryRowByAppId(appID);
    if (!row.has_value()) {
        return false;
    }

    std::string controllerError;
    const bool active = runtimeController->isUnitActive(buildUnitName(appID), &controllerError);
    if (!active && !controllerError.empty()) {
        CubeLog::debug("AppsManager: app not active (" + appID + "): " + controllerError);
    }
    return active;
}

bool AppsManager::isAppInstalled(const std::string& appID) const
{
    if (!waitForDatabaseManager()) {
        return false;
    }

    auto row = getRegistryRowByAppId(appID);
    if (!row.has_value()) {
        return false;
    }

    std::error_code ec;
    const bool manifestExists = fs::exists(row->manifestPath, ec) && !ec;
    ec.clear();
    const bool installRootExists = fs::exists(row->installRoot, ec) && !ec;
    return manifestExists && installRootExists;
}

std::vector<std::string> AppsManager::getAppIDs() const
{
    std::vector<std::string> appIds;
    if (!waitForDatabaseManager()) {
        return appIds;
    }

    auto* db = CubeDB::getDBManager()->getDatabase(kAppsDatabaseName);
    if (!db) {
        return appIds;
    }

    const auto rows = db->selectData(kAppsTableName, { "app_id" });
    appIds.reserve(rows.size());
    for (const auto& row : rows) {
        if (!row.empty()) {
            appIds.push_back(row[0]);
        }
    }

    return appIds;
}

std::vector<std::string> AppsManager::getAppNames() const
{
    std::vector<std::string> appNames;
    if (!waitForDatabaseManager()) {
        return appNames;
    }

    auto* db = CubeDB::getDBManager()->getDatabase(kAppsDatabaseName);
    if (!db) {
        return appNames;
    }

    const auto rows = db->selectData(kAppsTableName, { "app_name" });
    appNames.reserve(rows.size());
    for (const auto& row : rows) {
        if (!row.empty()) {
            appNames.push_back(row[0]);
        }
    }

    return appNames;
}

std::vector<std::string> AppsManager::getAppNames_static()
{
    std::vector<std::string> appNames;
    try {
        auto dbManager = CubeDB::getDBManager();
        if (dbManager == nullptr || !dbManager->isDatabaseManagerReady()) {
            return appNames;
        }

        auto* db = dbManager->getDatabase(kAppsDatabaseName);
        if (!db) {
            return appNames;
        }

        const auto rows = db->selectData(kAppsTableName, { "app_name" });
        appNames.reserve(rows.size());
        for (const auto& row : rows) {
            if (!row.empty()) {
                appNames.push_back(row[0]);
            }
        }
    } catch (...) {
        CubeLog::warning("AppsManager::getAppNames_static failed");
    }

    return appNames;
}
