#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <logger.h>
#include "./../database/cubeDB.h"

std::string KeyGenerator(size_t length);
std::string Code6Generator();

class CubeAuth{
public:
    CubeAuth();
    ~CubeAuth();
    bool checkAuth(std::string privateKey, std::string app_id);
    std::string generateKey();
    std::string generateCode6();
    std::pair<std::string,std::string> generateOpenSSLKeys();
};