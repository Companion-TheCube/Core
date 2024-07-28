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

bool NativeAPI::isProcessRunning(long pid)
{
    if (kill(pid, 0) == 0) {
        return true;
    }
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

bool NativeAPI::isProcessRunning(long pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == NULL) {
        return false;
    }
    DWORD exitCode;
    if (GetExitCodeProcess(hProcess, &exitCode) == 0) {
        return false;
    }
    if (exitCode == STILL_ACTIVE) {
        return true;
    }
    return false;
}
#endif

RunningApp* NativeAPI::startApp(std::string execPath, std::string execArgs, std::string appID, std::string appName, std::string appSource, std::string updatePath)
{
    CubeLog::info("Starting app: " + execPath + " " + execArgs);
    std::string cwd = std::filesystem::current_path().string();
    std::string fullPath = cwd + "/" + execPath;
#ifdef __linux__
    if(fullPath.find("\\") != std::string::npos){
        fullPath = std::regex_replace(fullPath, std::regex("\\\\"), "/");
    }   
#endif
    std::filesystem::path p = std::filesystem::path(fullPath);
    CubeLog::critical("App path: " + p.string());
    if (!std::filesystem::exists(p)) {
        CubeLog::error("App not found: " + execPath);
        return nullptr;
    }
    std::string command = execPath + " " + execArgs;

    RunningApp* temp = new RunningApp(0, appID, appName, execPath, execArgs, appSource, updatePath, "native", "", "", "", "", 0);
#ifdef _WIN32
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

    CubeLog::debug("Creating pipes for stdin");
    if (!CreatePipe(temp->getStdInRead(), temp->getStdInWrite(), &saAttr, 0)) {
        CubeLog::error("Stdin pipe creation failed");
        return nullptr;
    }

    CubeLog::debug("Setting stdin pipe information");
    if (!SetHandleInformation(temp->getStdInWriteHandle(), HANDLE_FLAG_INHERIT, 0)) {
        CubeLog::error("Stdin SetHandleInformation failed");
        return nullptr;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    si.hStdError = temp->getStdErrWriteHandle();
    si.hStdOutput = temp->getStdOutWriteHandle();
    si.hStdInput = temp->getStdInReadHandle();
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
        // while(true){
        //     genericSleep(100);
        // };
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        // CloseHandle(temp->getStdOutWriteHandle());
        // CloseHandle(temp->getStdErrWriteHandle());

        return temp;
    }
#endif
#ifdef __linux__
    pid_t pid = 0;
    std::string execCommand = execPath + " " + execArgs;
    CubeLog::debug("Exec command: " + execCommand);
    const char *path = execPath.c_str();
    char *const argv[] = { (char*)path, (char*)"arg1", NULL };
    // add the current working directory to the path
    std::string path_str = std::string(cwd + "/" + execPath).c_str();
    int status = posix_spawn(&pid, path_str.c_str(), NULL, NULL, argv, environ);

    if (status == 0) {
        temp->setPID(pid);
        return temp;
    } else {
        std::cerr << "posix_spawn failed: " << status << std::endl;
    }
    return nullptr;
#endif
#ifndef _WIN32
#ifndef __linux__
    CubeLog::error("Unsupported platform");
    return nullptr;
#endif
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
        int pid = execl(command.c_str(), "", NULL);
        CubeLog::debug("PID for app: " + execPath + " is " + std::to_string(pid));
        return pid;
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

bool NativeAPI::isExecutableInstalled(std::string execPath)
{
    CubeLog::info("Checking if app is installed: " + execPath);
    std::filesystem::path p = std::filesystem::path(execPath);
    if (!std::filesystem::exists(p)) {
        CubeLog::error("App not found: " + execPath);
        return false;
    } else {
        CubeLog::info("App found: " + execPath);
        return true;
    }
}