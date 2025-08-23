/*
██╗   ██╗████████╗██╗██╗     ███████╗    ██████╗██████╗ ██████╗ 
██║   ██║╚══██╔══╝██║██║     ██╔════╝   ██╔════╝██╔══██╗██╔══██╗
██║   ██║   ██║   ██║██║     ███████╗   ██║     ██████╔╝██████╔╝
██║   ██║   ██║   ██║██║     ╚════██║   ██║     ██╔═══╝ ██╔═══╝ 
╚██████╔╝   ██║   ██║███████╗███████║██╗╚██████╗██║     ██║     
 ╚═════╝    ╚═╝   ╚═╝╚══════╝╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
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

#ifndef LOGGER_H
#include <logger.h>
#endif
#include "utils.h"

void genericSleep(int ms)
{
    usleep(ms * 1000);
}

void monitorMemoryAndCPU()
{
    CubeLog::info("Memory: " + getMemoryFootprint());
    CubeLog::info("CPU: " + getCpuUsage());
}

std::string getMemoryFootprint()
{
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
}

std::string getCpuUsage()
{
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
}

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
/**
 * @brief Base64 decode a string. Based on code from https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp
 *
 * @param encoded_string
 * @return std::vector<unsigned char>
 */
std::vector<unsigned char> base64_decode_cube(const std::string& encoded_string)
{
    return cppcodec::base64_rfc4648::decode(encoded_string);
}


/**
 * @brief Base64 encode a string. Based on code from https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp
 *
 * @param bytes_to_encode
 * @return std::string
 */
std::string base64_encode_cube(const std::vector<unsigned char>& bytes_to_encode)
{
    return cppcodec::base64_rfc4648::encode(bytes_to_encode);
}

/**
 * @brief Base64 encode a string. Based on code from https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp
 *
 * @param bytes_to_encode std::string
 * @return std::string
 */
std::string base64_encode_cube(const std::string& bytes_to_encode)
{
    return cppcodec::base64_rfc4648::encode(std::vector<unsigned char>(bytes_to_encode.begin(), bytes_to_encode.end()));
}