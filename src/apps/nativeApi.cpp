/*
███╗   ██╗ █████╗ ████████╗██╗██╗   ██╗███████╗ █████╗ ██████╗ ██╗    ██████╗██████╗ ██████╗ 
████╗  ██║██╔══██╗╚══██╔══╝██║██║   ██║██╔════╝██╔══██╗██╔══██╗██║   ██╔════╝██╔══██╗██╔══██╗
██╔██╗ ██║███████║   ██║   ██║██║   ██║█████╗  ███████║██████╔╝██║   ██║     ██████╔╝██████╔╝
██║╚██╗██║██╔══██║   ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██║██╔═══╝ ██║   ██║     ██╔═══╝ ██╔═══╝ 
██║ ╚████║██║  ██║   ██║   ██║ ╚████╔╝ ███████╗██║  ██║██║     ██║██╗╚██████╗██║     ██║     
╚═╝  ╚═══╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═══╝  ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
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

#include "nativeApi.h"

bool NativeAPI::isProcessRunning(const std::string& processName)
{
    DIR* dir = opendir("/proc");
    if (dir == nullptr) {
        perror("opendir");
        return false;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR) {
            std::string pidDir = entry->d_name;
            if (pidDir.find_first_not_of("0123456789") == std::string::npos) {
                std::ifstream cmdline("/proc/" + pidDir + "/cmdline");
                std::string cmd;
                std::getline(cmdline, cmd);
                if (cmd.find(processName) != std::string::npos) {
                    closedir(dir);
                    cmdline.close();
                    return true;
                } else {
                    cmdline.close();
                }
            }
        }
    }

    closedir(dir);
    return false;
}

bool NativeAPI::isProcessRunning(long pid)
{
    // check if the process is running
    DIR* dir = opendir("/proc");
    if (dir == nullptr) {
        perror("opendir");
        return false;
    }
    std::vector<std::string> dirs;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR) {
            std::string pidDir = entry->d_name;
            dirs.push_back(pidDir);
            if (pidDir.find_first_not_of("0123456789") == std::string::npos) {
                if (std::stol(pidDir) == pid) {
                    closedir(dir);
                    return true;
                }
            }
        }
    }
    closedir(dir);
    CubeLog::debug("Process with PID " + std::to_string(pid) + " is NOT running");
    return false;
}



/**
 * @brief Start a native app
 * 
 * @param execPath 
 * @param execArgs 
 * @param appID 
 * @param appName 
 * @param appSource 
 * @param updatePath 
 * @return std::unique_ptr<RunningApp> 
 */
std::unique_ptr<RunningApp> NativeAPI::startApp(const std::string& execPath, const std::string& execArgs, const std::string& appID, const std::string& appName, const std::string& appSource, const std::string& updatePath)
{
    CubeLog::info("Starting app: " + execPath + " " + execArgs);
    std::filesystem::path cwd = std::filesystem::current_path();
    std::filesystem::path fullPath = cwd / execPath;
    // std::replace(ePath.begin(), ePath.end(), '\\', '/');
    // std::filesystem::path p = std::filesystem::path(ePath);
    if (!std::filesystem::exists(fullPath)) {
        CubeLog::error("App not found: " + fullPath.string());
        return nullptr;
    }
    std::string command = execPath + " " + execArgs;

    auto temp = std::make_unique<RunningApp>(0, appID, appName, execPath, execArgs, appSource, updatePath, "native", "", "", "", "", 0);

    pid_t pid = 0;
    std::string execCommand = fullPath.string() + " " + execArgs;
    CubeLog::debug("Exec command: " + execCommand);
    const char* path = execPath.c_str();
    std::vector<std::string> args;
    // split execArgs by space
    std::istringstream iss(execArgs);
    for (std::string s; iss >> s;) {
        args.push_back(s);
    }
    char* argv[args.size() + 2];
    argv[0] = (char*)path;
    for (size_t i = 0; i < args.size(); i++) {
        argv[i + 1] = (char*)args[i].c_str();
    }
    argv[args.size() + 1] = NULL;

    // setup stdout and stderr
    int stdoutPipe[2];
    if (pipe(stdoutPipe) == -1) {
        CubeLog::error("Failed to create stdout pipe");
        return nullptr;
    }
    int stderrPipe[2];
    if (pipe(stderrPipe) == -1) {
        CubeLog::error("Failed to create stderr pipe");
        return nullptr;
    }
    int stdinPipe[2];
    if (pipe(stdinPipe) == -1) {
        CubeLog::error("Failed to create stdin pipe");
        return nullptr;
    }
    temp->setStdOutRead(stdoutPipe[0]);
    temp->setStdOutWrite(stdoutPipe[1]);
    temp->setStdInRead(stdinPipe[0]);
    temp->setStdInWrite(stdinPipe[1]);
    temp->setStdErrRead(stderrPipe[0]);
    temp->setStdErrWrite(stderrPipe[1]);

    posix_spawn_file_actions_init(temp->getActions());

    posix_spawn_file_actions_adddup2(temp->getActions(), temp->getStdOutWrite(), STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(temp->getActions(), temp->getStdErrWrite(), STDERR_FILENO);
    posix_spawn_file_actions_adddup2(temp->getActions(), temp->getStdInRead(), STDIN_FILENO);
    posix_spawn_file_actions_addclose(temp->getActions(), temp->getStdInWrite());
    posix_spawn_file_actions_addclose(temp->getActions(), temp->getStdOutRead());
    posix_spawn_file_actions_addclose(temp->getActions(), temp->getStdErrRead());

    int status = posix_spawn(&pid, fullPath.c_str(), temp->getActions(), NULL, const_cast<char* const*>(argv), environ);

    if (status != 0) {
        CubeLog::error("Failed to start app: " + execPath + " " + execArgs);
        return nullptr;
    }
    // close the write end of the pipes
    close(temp->getStdOutWrite());
    close(temp->getStdErrWrite());
    close(temp->getStdInRead());
    temp->setPID(pid);
    CubeLog::debug("Process created with PID: " + std::to_string(pid));
    CubeLog::info("App started: " + execPath + " " + execArgs);
    return temp;
}

bool NativeAPI::stopApp(const std::string& execPath)
{
    CubeLog::info("Stopping app: " + execPath);
    std::string ePath = execPath;
    std::replace(ePath.begin(), ePath.end(), '\\', '/');
    if (isProcessRunning(ePath)) {
        std::string command = "pkill " + ePath;
        if (system(command.c_str()) == -1) {
            CubeLog::error("Failed to stop app: " + ePath);
            return false;
        }
        if (isProcessRunning(ePath)) {
            CubeLog::error("Failed to stop app: " + ePath);
            return false;
        } else {
            CubeLog::info("App stopped: " + ePath);
            return true;
        }
    } else {
        CubeLog::error("App is not running: " + ePath);
        return false;
    }
}

bool NativeAPI::stopApp(long pid)
{
    CubeLog::info("Stopping app with PID: " + std::to_string(pid));
    int status = kill(pid, SIGKILL);
    if (status == -1) {
        CubeLog::error("Failed to stop app with PID=" + std::to_string(pid) + ". Error: " + std::to_string(errno));
        CubeLog::error("status: " + std::to_string(status));
        return false;
    }
    if (isProcessRunning(pid)) {
        CubeLog::error("Failed to stop app with PID: " + std::to_string(pid));
        return false;
    } else {
        CubeLog::info("App stopped: " + std::to_string(pid));
        return true;
    }
}


long NativeAPI::getPID(const std::string& execPath)
{
    CubeLog::info("Getting PID for app: " + execPath);
    if (isProcessRunning(execPath)) {
        std::string command = "pidof " + execPath;
        int pid = execl(command.c_str(), "", NULL);
        CubeLog::debug("PID for app: " + execPath + " is " + std::to_string(pid));
        return pid;
    } else {
        CubeLog::error("App is not running: " + execPath);
        return -1;
    }
}


bool NativeAPI::isExecutableInstalled(const std::string& execPath)
{
    std::filesystem::path p = std::filesystem::path(std::regex_replace(execPath, std::regex("\\\\"), "/"));

    if (!std::filesystem::exists(p)) {
        return false;
    } else {
        return true;
    }
}