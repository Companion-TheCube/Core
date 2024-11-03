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
        // CubeLog::debugSilly("Checking process: " + convertWCHARToString(pe32.szExeFile));
        if (processName == convertWCHARToString(pe32.szExeFile)) {
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

std::unique_ptr<RunningApp> NativeAPI::startApp(const std::string& execPath, const std::string& execArgs, const std::string& appID, const std::string& appName, const std::string& appSource, const std::string& updatePath)
{
    CubeLog::info("Starting app: " + execPath + " " + execArgs);
    std::string cwd = std::filesystem::current_path().string();
    std::string fullPath = cwd + "/" + execPath;
    std::string ePath = execPath;
#ifdef __linux__
    std::replace(ePath.begin(), ePath.end(), '\\', '/');
#endif
    std::filesystem::path p = std::filesystem::path(ePath);
    if (!std::filesystem::exists(p)) {
        CubeLog::error("App not found: " + p.string());
        return nullptr;
    }
    std::string command = execPath + " " + execArgs;

    auto temp = std::make_unique<RunningApp>(0, appID, appName, execPath, execArgs, appSource, updatePath, "native", "", "", "", "", 0);
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

    cwd = std::filesystem::current_path().string();
    std::string execCommand = cwd + "\\" + execPath + " " + execArgs;
    CubeLog::debug("Exec command: " + execCommand);
    WCHAR tempCommand[1024];
    convertStringToWCHAR(execCommand, tempCommand);
    if (!CreateProcess(NULL,
            tempCommand, // command line
            NULL, // process security attributes
            NULL, // primary thread security attributes
            TRUE, // handles are inherited
            0, // creation flags
            NULL, // use parent's environment
            NULL, // use parent's current directory
            &si, // STARTUPINFO pointer
            &pi)) { // receives PROCESS_INFORMATION
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
    pid_t pid = 0;
    std::string execCommand = execPath + " " + execArgs;
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
    // add the current working directory to the path
    std::string path_str = std::string(cwd + "/" + execPath).c_str();

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

    int status = posix_spawn(&pid, path_str.c_str(), temp->getActions(), NULL, const_cast<char* const*>(argv), environ);

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
#endif
#ifndef _WIN32
#ifndef __linux__
    CubeLog::error("Unsupported platform");
    return nullptr;
#endif
#endif
}

bool NativeAPI::stopApp(const std::string& execPath)
{
    CubeLog::info("Stopping app: " + execPath);
    std::string ePath = execPath;
#ifdef __linux__
    std::replace(ePath.begin(), ePath.end(), '\\', '/');
#endif
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
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL) {
        CubeLog::error("Failed to stop app with PID: " + std::to_string(pid));
        return false;
    }
    if (TerminateProcess(hProcess, 0) == 0) {
        CubeLog::error("Failed to stop app with PID: " + std::to_string(pid));
        return false;
    }
#endif
#ifdef __linux__
    int status = kill(pid, SIGKILL);
    if (status == -1) {
        CubeLog::error("Failed to stop app with PID=" + std::to_string(pid) + ". Error: " + std::to_string(errno));
        CubeLog::error("status: " + std::to_string(status));
        return false;
    }
#endif
    if (isProcessRunning(pid)) {
        CubeLog::error("Failed to stop app with PID: " + std::to_string(pid));
        return false;
    } else {
        CubeLog::info("App stopped: " + std::to_string(pid));
        return true;
    }
}

#ifdef __linux__
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
#endif

#ifdef _WIN32
long NativeAPI::getPID(const std::string& execPath)
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
            // CubeLog::debugSilly("Checking process: " + convertWCHARToString(pe32.szExeFile));
            if (execPath == convertWCHARToString(pe32.szExeFile)) {
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

bool NativeAPI::isExecutableInstalled(const std::string& execPath)
{
#ifdef __linux__
    std::filesystem::path p = std::filesystem::path(std::regex_replace(execPath, std::regex("\\\\"), "/"));
#else
    std::filesystem::path p = std::filesystem::path(execPath);
#endif
    if (!std::filesystem::exists(p)) {
        return false;
    } else {
        return true;
    }
}