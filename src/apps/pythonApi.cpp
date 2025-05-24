#include "pythonApi.h"

bool PythonAPI::isProcessRunning(long pid)
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

bool PythonAPI::isProcessRunning(const std::string& scriptName, const std::string& execArgs)
{
    // check if the process is running
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
                if (cmd.find(scriptName) != std::string::npos && cmd.find(execArgs) != std::string::npos) {
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
bool PythonAPI::stopApp(long pid)
{
    CubeLog::info("Stopping app with PID: " + std::to_string(pid));
    if (isProcessRunning(pid)) {
        std::string command = "kill -9 " + std::to_string(pid);
        if (system(command.c_str()) == -1) {
            CubeLog::error("Failed to stop app with PID: " + std::to_string(pid));
            return false;
        }
        if (isProcessRunning(pid)) {
            CubeLog::error("Failed to stop app with PID: " + std::to_string(pid));
            return false;
        } else {
            CubeLog::info("App stopped: " + std::to_string(pid));
            return true;
        }
    } else {
        CubeLog::error("App is not running: " + std::to_string(pid));
        return false;
    }
}

std::unique_ptr<RunningApp> PythonAPI::startApp(const std::string& execPath, const std::string& execArgs, const std::string& appID, const std::string& appName, const std::string& appSource, const std::string& updatePath)
{
    CubeLog::info("Starting app: " + execPath + " " + execArgs);
    std::filesystem::path cwd = std::filesystem::current_path();
    std::filesystem::path fullPath = cwd / execPath;
    if (!std::filesystem::exists(fullPath)) {
        CubeLog::error("App not found: " + fullPath.string());
        return nullptr;
    }
    std::string command = execPath + " " + execArgs;
    auto temp = std::make_unique<RunningApp>(0, appID, appName, execPath, execArgs, appSource, updatePath, "native", "", "", "", "", 0);
    pid_t pid = fork();
    if (pid == -1) {
        CubeLog::error("Failed to fork process");
        return nullptr;
    } else if (pid == 0) {
        // Child process
        char* args[] = { (char*)execPath.c_str(), (char*)execArgs.c_str(), NULL };
        dup2(temp->getStdInWrite(), STDIN_FILENO);
        dup2(temp->getStdOutWrite(), STDOUT_FILENO);
        dup2(temp->getStdErrWrite(), STDERR_FILENO);
        close(temp->getStdInWrite());
        close(temp->getStdOutWrite());
        close(temp->getStdErrWrite());
        execv(execPath.c_str(), args);
        exit(0);
    } else {
        // Parent process
        temp->setPID(pid);
        CubeLog::debug("Process created with PID: " + std::to_string(pid));
        CubeLog::info("App started: " + execPath + " " + execArgs);
        return temp;
    }
}

long PythonAPI::getPID(const std::string& execPath, const std::string& execArgs)
{
    CubeLog::info("Getting PID for app: " + execPath + " " + execArgs);
    if (isProcessRunning(execPath, execArgs)) {
        std::string command = "pidof " + execPath;
        int pid = execl(command.c_str(), "", NULL);
        CubeLog::debug("PID for app: " + execPath + " is " + std::to_string(pid));
        return pid;
    } else {
        CubeLog::error("App is not running: " + execPath);
        return -1;
    }
}

