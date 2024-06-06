#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <logger.h>
#include "./../database/cubeDB.h"
#include <sodium.h>
#include <random>

#define CUBE_APPS_ID_LENGTH 64

class CubeAuth{
    static bool available;
    static std::string privateKey;
    static std::string publicKey;
public:
    CubeAuth();
    ~CubeAuth();
    int checkAuth(std::string privateKey, std::string app_id, std::string encrypted_auth_code);
    std::string generateAuthCode();
    std::pair<std::string,std::string> generateKeyPair();
    std::string encryptAuthCode(std::string auth_code, std::string public_key);
    std::string decryptAuthCode(std::string auth_code, std::string private_key);
    std::string encryptData(std::string data, std::string public_key);
    std::string decryptData(std::string data, std::string private_key, size_t length);

    enum AUTH_CODES{
        AUTH_FAIL_UNKNOWN = -1,
        AUTH_SUCCESS = 0,
        AUTH_FAIL_INVALID_PRIVATE_KEY = 1,
        AUTH_FAIL_INVALID_APP_ID_LEN = 2,
        AUTH_FAIL_DB_NOT_OPEN = 3,
        AUTH_FAIL_INVALID_APP_ID = 4,
        AUTH_FAIL_KEY_GEN = 5,
        AUTH_FAIL_DECRYPT = 6,
        AUTH_FAIL_CODE_MISMATCH = 7
    };
};