// #pragma once
#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H
#include <expected>
#include <filesystem>
#include <iostream>
#include <logger.h>
#include <random>
#include <sodium.h>
#include <string>
#include <vector>
#ifndef CUBE_DB_H
#include "./../database/cubeDB.h"
#endif
#ifndef API_I_H
#include "api_i.h"
#endif
#ifndef GUI_H
#include "./../gui/gui.h"
#endif

#define CUBE_APPS_ID_LENGTH 64

class CubeAuth : public I_API_Interface {
    static bool available;
    static std::string privateKey;
    static std::string publicKey;
    static bool cubeAuthStaticKeysSet;
    static std::string lastError;

public:
    CubeAuth();
    ~CubeAuth();
    enum class AUTH_CODE {
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
    static AUTH_CODE checkAuth(const std::string& privateKey, const std::string& app_id, const std::string& encrypted_auth_code);
    static bool isAuthorized_authHeader(const std::string& authHeader);
    static std::string generateAuthCode();
    static std::pair<std::string, std::string> generateKeyPair();
    static std::string encryptAuthCode(const std::string& auth_code, const std::string& public_key);
    static std::string decryptAuthCode(const std::string& auth_code, const std::string& private_key);
    static std::string encryptData(const std::string& data, const std::string& public_key);
    static std::string decryptData(const std::string& data, const std::string& private_key, size_t length);
    static std::string getLastError();

    // API Interface
    constexpr std::string getInterfaceName() const override;
    HttpEndPointData_t getHttpEndpointData();
};

#endif