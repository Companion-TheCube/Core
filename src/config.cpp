/*
 ██████╗ ██████╗ ███╗   ██╗███████╗██╗ ██████╗     ██████╗██████╗ ██████╗ 
██╔════╝██╔═══██╗████╗  ██║██╔════╝██║██╔════╝    ██╔════╝██╔══██╗██╔══██╗
██║     ██║   ██║██╔██╗ ██║█████╗  ██║██║  ███╗   ██║     ██████╔╝██████╔╝
██║     ██║   ██║██║╚██╗██║██╔══╝  ██║██║   ██║   ██║     ██╔═══╝ ██╔═══╝ 
╚██████╗╚██████╔╝██║ ╚████║██║     ██║╚██████╔╝██╗╚██████╗██║     ██║     
 ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝     ╚═╝ ╚═════╝ ╚═╝ ╚═════╝╚═╝     ╚═╝
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


#include "utils.h"
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <cstdlib>

namespace {
std::unordered_map<std::string, std::string> g_cfg;
std::once_flag g_loaded;

static std::string trim(const std::string& s)
{
    size_t b = s.find_first_not_of(" \t\r\n");
    size_t e = s.find_last_not_of(" \t\r\n");
    if (b == std::string::npos) return std::string();
    return s.substr(b, e - b + 1);
}

static void ensure_loaded()
{
    std::call_once(g_loaded, []() {
        Config::loadFromDotEnv(".env");
    });
}
}

namespace Config {

void loadFromDotEnv(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return;
    std::string line;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        auto key = trim(line.substr(0, pos));
        auto val = trim(line.substr(pos + 1));
        if (!val.empty() && ((val.front() == '"' && val.back() == '"') || (val.front() == '\'' && val.back() == '\''))) {
            val = val.substr(1, val.size() - 2);
        }
        // Environment variables override .env values
        if (const char* envp = std::getenv(key.c_str())) {
            g_cfg[key] = std::string(envp);
        } else {
            g_cfg[key] = val;
        }
    }
}

std::string get(const std::string& key, const std::string& defaultValue)
{
    ensure_loaded();
    auto it = g_cfg.find(key);
    if (it != g_cfg.end()) return it->second;
    if (const char* envp = std::getenv(key.c_str())) return std::string(envp);
    return defaultValue;
}

void set(const std::string& key, const std::string& value)
{
    ensure_loaded();
    g_cfg[key] = value;
}

bool has(const std::string& key)
{
    ensure_loaded();
    if (g_cfg.count(key)) return true;
    return std::getenv(key.c_str()) != nullptr;
}

} // namespace Config

