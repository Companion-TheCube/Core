#ifndef LOGGER_H
#include <logger.h>
#endif
#include "utils.h"

void genericSleep(int ms)
{
#ifdef __linux__
    usleep(ms * 1000);
#endif
#ifdef _WIN32
    Sleep(ms);
#endif
}

void monitorMemoryAndCPU()
{
    CubeLog::info("Memory: " + getMemoryFootprint());
    CubeLog::info("CPU: " + getCpuUsage());
}

std::string getMemoryFootprint()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return std::to_string(pmc.WorkingSetSize / 1024) + " KB";
#endif
#ifdef __linux__
    std::ifstream file("/proc/self/status");
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("VmRSS") != std::string::npos) {
            std::string rss = line.substr(line.find(":") + 1);
            rss.erase(std::remove_if(rss.begin(), rss.end(), isspace), rss.end());
            return rss;
        }
    }
    return "0";
#endif
}

std::string getCpuUsage()
{
#ifdef _WIN32
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime) == 0) {
        return "0";
    }
    static ULONGLONG lastIdleTime = 0;
    static ULONGLONG lastKernelTime = 0;
    static ULONGLONG lastUserTime = 0;
    ULONGLONG idle = (reinterpret_cast<ULARGE_INTEGER*>(&idleTime)->QuadPart - lastIdleTime);
    ULONGLONG kernel = (reinterpret_cast<ULARGE_INTEGER*>(&kernelTime)->QuadPart - lastKernelTime);
    ULONGLONG user = (reinterpret_cast<ULARGE_INTEGER*>(&userTime)->QuadPart - lastUserTime);
    ULONGLONG total = kernel + user;
    lastIdleTime = reinterpret_cast<ULARGE_INTEGER*>(&idleTime)->QuadPart;
    lastKernelTime = reinterpret_cast<ULARGE_INTEGER*>(&kernelTime)->QuadPart;
    lastUserTime = reinterpret_cast<ULARGE_INTEGER*>(&userTime)->QuadPart;
    std::string returnString = "Overall CPU: " + std::to_string((total - idle) * 100 / total) + "%   ";

    static ULONGLONG lastProcessKernelTime = 0;
    static ULONGLONG lastProcessUserTime = 0;
    static ULONGLONG lastTime = 0;

    FILETIME ftCreation, ftExit, ftKernel, ftUser;
    ULARGE_INTEGER now, processKernelTime, processUserTime;

    // Get current process times
    if (!GetProcessTimes(GetCurrentProcess(), &ftCreation, &ftExit, &ftKernel, &ftUser)) {
        returnString += "Process CPU: 0%";
        return returnString;
    }

    processKernelTime.QuadPart = reinterpret_cast<ULARGE_INTEGER*>(&ftKernel)->QuadPart;
    processUserTime.QuadPart = reinterpret_cast<ULARGE_INTEGER*>(&ftUser)->QuadPart;

    // Get the current time
    FILETIME ftNow;
    GetSystemTimeAsFileTime(&ftNow);
    now.QuadPart = reinterpret_cast<ULARGE_INTEGER*>(&ftNow)->QuadPart;

    if (lastTime != 0) {
        ULONGLONG systemTime = now.QuadPart - lastTime;
        ULONGLONG processTime = (processKernelTime.QuadPart - lastProcessKernelTime) +
                                (processUserTime.QuadPart - lastProcessUserTime);

        // Get the number of logical processors
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        DWORD numProcessors = sysInfo.dwNumberOfProcessors;

        double cpuUsage = (double(processTime) * 100.0) / (double(systemTime) * double(numProcessors));

        // Update last times
        lastProcessKernelTime = processKernelTime.QuadPart;
        lastProcessUserTime = processUserTime.QuadPart;
        lastTime = now.QuadPart;

        // get cpuUsage as a float with 2 decimal places
        cpuUsage = std::floor(cpuUsage * 100 + 0.5) / 100;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << cpuUsage;
        std::string cpuUsageString = ss.str();

        returnString += "Process CPU: " + cpuUsageString + "%";
        return returnString;
    }

    // First call, initialize last times
    lastProcessKernelTime = processKernelTime.QuadPart;
    lastProcessUserTime = processUserTime.QuadPart;
    lastTime = now.QuadPart;

    returnString += "Process CPU: 0%";

    return returnString;
#endif
#ifdef __linux__
    std::ifstream file("/proc/stat");
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("cpu") != std::string::npos) {
            std::istringstream ss(line);
            std::vector<std::string> tokens;
            std::string token;
            while (std::getline(ss, token, ' ')) {
                tokens.push_back(token);
            }
            // TODO: fix this
            // long user = std::stol(tokens[1]);
            // long nice = std::stol(tokens[2]);
            // long system = std::stol(tokens[3]);
            // long idle = std::stol(tokens[4]);
            // long iowait = std::stol(tokens[5]);
            // long irq = std::stol(tokens[6]);
            // long softirq = std::stol(tokens[7]);
            // long steal = std::stol(tokens[8]);
            // long guest = std::stol(tokens[9]);
            // long guest_nice = std::stol(tokens[10]);
            // long total = user + nice + system + idle + iowait + irq + softirq + steal + guest + guest_nice;
            // long idleTotal = idle + iowait;
            // return std::to_string((total - idleTotal) * 100 / total);
            return "";
        }
    }
    return "0";
#endif
}

#ifdef _WIN32
std::string convertWCHARToString(const WCHAR* wstr) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

void convertStringToWCHAR(const std::string& str, WCHAR* wstr) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wstrTemp = converter.from_bytes(str);
    wcscpy(wstr, wstrTemp.c_str());
}
#endif

std::string sha256(std::string input){
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.length());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string crc32(std::string input){
    // crc32 without using library
    unsigned int crc = 0xFFFFFFFF;
    for (size_t i = 0; i < input.size(); i++) {
        crc = crc ^ input[i];
        for (size_t j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    crc = ~crc;
    std::stringstream ss;
    ss << std::hex << crc;
    return ss.str();
}