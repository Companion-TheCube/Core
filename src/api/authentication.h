//#pragma once
#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <logger.h>
#include <sodium.h>
#include <random>
#include <expected>
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
    static int checkAuth(std::string privateKey, std::string app_id, std::string encrypted_auth_code);
    static bool isAuthorized_authHeader(std::string authHeader);
    static std::string generateAuthCode();
    static std::pair<std::string, std::string> generateKeyPair();
    static std::string encryptAuthCode(std::string auth_code, std::string public_key);
    static std::string decryptAuthCode(std::string auth_code, std::string private_key);
    static std::string encryptData(std::string data, std::string public_key);
    static std::string decryptData(std::string data, std::string private_key, size_t length);
    static std::string getLastError();

    // API Interface
    std::string getIntefaceName() const;
    HttpEndPointData_t getHttpEndpointData();
    std::vector<std::pair<std::string, std::vector<std::string>>> getHttpEndpointNamesAndParams();

    enum AUTH_CODES {
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

#endif