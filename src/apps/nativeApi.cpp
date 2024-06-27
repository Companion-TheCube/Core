#include "nativeApi.h"

#ifdef __linux__
bool NativeAPI::isProcessRunning(const std::string& processName) {
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
                    return true;
                }
            }
        }
    }

    closedir(dir);
    return false;
}
#endif

#ifdef _WIN32
bool NativeAPI::isProcessRunning(const std::string& processName) {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    bool found = false;
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateToolhelp32Snapshot failed: " << GetLastError() << std::endl;
        return false;
    }
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hProcessSnap, &pe32)) {
        std::cerr << "Process32First failed: " << GetLastError() << std::endl;
        CloseHandle(hProcessSnap);
        return false;
    }
    do {
        if (processName == pe32.szExeFile) {
            found = true;
            break;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return found;
}
#endif

long NativeAPI::startApp(std::string execPath, std::string execArgs) {
    CubeLog::info("Starting app: " + execPath + " " + execArgs);
    std::filesystem::path p = std::filesystem::path(execPath);
    if (!std::filesystem::exists(p)) {
        CubeLog::error("App not found: " + execPath);
        return false;
    }
    std::string command = execPath + " " + execArgs;
    auto app = command.c_str();
    if(system(app) == -1) {
        CubeLog::error("Failed to start app: " + execPath);
        return false;
    }
    if(!isProcessRunning(execPath)) {
        CubeLog::error("Failed to start app: " + execPath);
        return false;
    }else{
        CubeLog::info("App started: " + execPath);
        return true;
    }
}

bool NativeAPI::stopApp(std::string execPath) {
    CubeLog::info("Stopping app: " + execPath);
    if(isProcessRunning(execPath)) {
        std::string command = "pkill " + execPath;
        if(system(command.c_str()) == -1) {
            CubeLog::error("Failed to stop app: " + execPath);
            return false;
        }
        if(isProcessRunning(execPath)) {
            CubeLog::error("Failed to stop app: " + execPath);
            return false;
        }else{
            CubeLog::info("App stopped: " + execPath);
            return true;
        }
    }else{
        CubeLog::error("App is not running: " + execPath);
        return false;
    }
    
}

#ifdef __linux__
long NativeAPI::getPID(std::string execPath) {
    CubeLog::info("Getting PID for app: " + execPath);
    if(isProcessRunning(execPath)) {
        std::string command = "pidof " + execPath;
        std::string pid = exec(command.c_str());
        CubeLog::debug("PID for app: " + execPath + " is " + pid);
        return std::stol(pid);
    }else{
        CubeLog::error("App is not running: " + execPath);
        return -1;
    }
}
#endif

#ifdef _WIN32
long NativeAPI::getPID(std::string execPath){
    CubeLog::info("Getting PID for app: " + execPath);
    if(isProcessRunning(execPath)) {
        HANDLE hProcessSnap;
        PROCESSENTRY32 pe32;
        hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {
            std::cerr << "CreateToolhelp32Snapshot failed: " << GetLastError() << std::endl;
            return -1;
        }
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (!Process32First(hProcessSnap, &pe32)) {
            std::cerr << "Process32First failed: " << GetLastError() << std::endl;
            CloseHandle(hProcessSnap);
            return -1;
        }
        do {
            if (execPath == pe32.szExeFile) {
                CubeLog::debug("PID for app: " + execPath + " is " + std::to_string(pe32.th32ProcessID));
                return pe32.th32ProcessID;
            }
        } while (Process32Next(hProcessSnap, &pe32));
        CloseHandle(hProcessSnap);
        return -1;
    }else{
        CubeLog::error("App is not running: " + execPath);
        return -1;
    }
}
#endif