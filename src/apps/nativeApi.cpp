#include "nativeApi.h"

#ifdef __linux__
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
bool NativeAPI::isProcessRunning(const std::string& processName)
{
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

RunningApp* NativeAPI::startApp(std::string execPath, std::string execArgs, std::string appID, std::string appName, std::string appSource, std::string updatePath)
{
    CubeLog::info("Starting app: " + execPath + " " + execArgs);
    std::filesystem::path p = std::filesystem::path(execPath);
    if (!std::filesystem::exists(p)) {
        CubeLog::error("App not found: " + execPath);
        return nullptr;
    }
    std::string command = execPath + " " + execArgs;
#ifdef _WIN32
    RunningApp* temp = new RunningApp(0, appID, appName, execPath, execArgs, appSource, updatePath, "native", "", "", "", "", 0);

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    CubeLog::debug("Creating pipes for stdout and stderr");
    if (!CreatePipe(temp->getStdOutRead(), temp->getStdOutWrite(), &saAttr, 0)) {
        CubeLog::error("Stdout pipe creation failed");
        return nullptr;
    }

    CubeLog::debug("Setting stdout pipe information");
    if (!SetHandleInformation(temp->getStdOutReadHandle(), HANDLE_FLAG_INHERIT, 0)) {
        CubeLog::error("Stdout SetHandleInformation failed");
        return nullptr;
    }

    CubeLog::debug("Creating pipes for stderr");
    if (!CreatePipe(temp->getStdErrRead(), temp->getStdErrWrite(), &saAttr, 0)) {
        CubeLog::error("Stderr pipe creation failed");
        return nullptr;
    }

    CubeLog::debug("Setting stderr pipe information");
    if (!SetHandleInformation(temp->getStdErrReadHandle(), HANDLE_FLAG_INHERIT, 0)) {
        CubeLog::error("Stderr SetHandleInformation failed");
        return nullptr;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    si.hStdError = temp->getStdErrWriteHandle();
    si.hStdOutput = temp->getStdOutWriteHandle();
    // disable stdinput
    si.hStdInput = NULL;
    si.dwFlags |= STARTF_USESTDHANDLES;

    std::string cwd = std::filesystem::current_path().string();
    std::string execCommand = cwd + "\\" + execPath + " " + execArgs;
    CubeLog::debug("Exec command: " + execCommand);
    if (!CreateProcess(NULL,
        (LPSTR)execCommand.c_str(), // command line
        NULL,                       // process security attributes
        NULL,                       // primary thread security attributes
        TRUE,                       // handles are inherited
        0,                          // creation flags
        NULL,                       // use parent's environment
        NULL,                       // use parent's current directory
        &si,                        // STARTUPINFO pointer
        &pi)) {                     // receives PROCESS_INFORMATION
        CubeLog::error("Error: " + std::to_string(GetLastError()));
        return nullptr;
    } else {
        temp->setPID(pi.dwProcessId);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return temp;
    }
#endif
#ifdef __linux__
    pid_t pid = fork();
    if (pid == 0) {
        std::string execCommand = execPath + " " + execArgs;
        CubeLog::debug("Exec command: " + execCommand);
        if (execl(execPath.c_str(), execPath.c_str(), execArgs.c_str(), NULL) == -1) {
            CubeLog::error("Error starting app. App_id: " + appID + ". App name: " + appName);
            CubeLog::error("Error: " + std::to_string(errno));
            return false;
        }
    } else if (pid < 0) {
        CubeLog::error("Error starting app. App_id: " + appID + ". App name: " + appName);
        CubeLog::error("Error: " + std::to_string(errno));
        return false;
    } else {
        CubeLog::info("App started successfully. App_id: " + appID + ". App name: " + appName);
        this->runningApps[appID] = new RunningApp(pid, appID, appName, execPath, execArgs, appSource, updatePath, role, "", "", "", "", 0);
        return true;
    }
#endif
}

bool NativeAPI::stopApp(std::string execPath)
{
    CubeLog::info("Stopping app: " + execPath);
    if (isProcessRunning(execPath)) {
        std::string command = "pkill " + execPath;
        if (system(command.c_str()) == -1) {
            CubeLog::error("Failed to stop app: " + execPath);
            return false;
        }
        if (isProcessRunning(execPath)) {
            CubeLog::error("Failed to stop app: " + execPath);
            return false;
        } else {
            CubeLog::info("App stopped: " + execPath);
            return true;
        }
    } else {
        CubeLog::error("App is not running: " + execPath);
        return false;
    }
}

bool NativeAPI::stopApp(long pid)
{
    CubeLog::info("Stopping app with PID: " + std::to_string(pid));
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL) {
        CubeLog::error("Failed to stop app with PID: " + std::to_string(pid));
        return false;
    }
    if (TerminateProcess(hProcess, 0) == 0) {
        CubeLog::error("Failed to stop app with PID: " + std::to_string(pid));
        return false;
    } else {
        CubeLog::info("App stopped: " + std::to_string(pid));
        return true;
    }
#endif
#ifdef __linux__
    if (kill(pid, SIGKILL) == -1) {
        CubeLog::error("Failed to stop app with PID: " + std::to_string(pid));
        return false;
    }
    if (kill(pid, 0) == 0) {
        CubeLog::error("Failed to stop app with PID: " + std::to_string(pid));
        return false;
    } else {
        CubeLog::info("App stopped: " + std::to_string(pid));
        return true;
    }
#endif
}

#ifdef __linux__
long NativeAPI::getPID(std::string execPath)
{
    CubeLog::info("Getting PID for app: " + execPath);
    if (isProcessRunning(execPath)) {
        std::string command = "pidof " + execPath;
        std::string pid = exec(command.c_str());
        CubeLog::debug("PID for app: " + execPath + " is " + pid);
        return std::stol(pid);
    } else {
        CubeLog::error("App is not running: " + execPath);
        return -1;
    }
}
#endif

#ifdef _WIN32
long NativeAPI::getPID(std::string execPath)
{
    CubeLog::info("Getting PID for app: " + execPath);
    if (isProcessRunning(execPath)) {
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
    } else {
        CubeLog::error("App is not running: " + execPath);
        return -1;
    }
}
#endif