/*
 █████╗ ██╗   ██╗████████╗██╗  ██╗███████╗███╗   ██╗████████╗██╗ ██████╗ █████╗ ████████╗██╗ ██████╗ ███╗   ██╗   ██╗  ██╗
██╔══██╗██║   ██║╚══██╔══╝██║  ██║██╔════╝████╗  ██║╚══██╔══╝██║██╔════╝██╔══██╗╚══██╔══╝██║██╔═══██╗████╗  ██║   ██║  ██║
███████║██║   ██║   ██║   ███████║█████╗  ██╔██╗ ██║   ██║   ██║██║     ███████║   ██║   ██║██║   ██║██╔██╗ ██║   ███████║
██╔══██║██║   ██║   ██║   ██╔══██║██╔══╝  ██║╚██╗██║   ██║   ██║██║     ██╔══██║   ██║   ██║██║   ██║██║╚██╗██║   ██╔══██║
██║  ██║╚██████╔╝   ██║   ██║  ██║███████╗██║ ╚████║   ██║   ██║╚██████╗██║  ██║   ██║   ██║╚██████╔╝██║ ╚████║██╗██║  ██║
╚═╝  ╚═╝ ╚═════╝    ╚═╝   ╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═╝ ╚═════╝╚═╝  ╚═╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝╚═╝  ╚═╝
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

// #pragma once
#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H
#include <expected>
#include <filesystem>
#include <iostream>
#include <random>
#include <sodium.h>
#include <string>
#include <vector>
#ifndef CUBE_DB_H
#include "./../database/cubeDB.h"
#endif
#ifndef API_H
#include "api.h"
#endif
#ifndef GUI_H
#include "./../gui/gui.h"
#endif

#define CUBE_APPS_ID_LENGTH 64

class CubeAuth : public AutoRegisterAPI<CubeAuth> {
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
    static bool ensureTokenTable();
    static std::string stripBearer(const std::string& header);
    static std::string generateAuthCode();
    static std::pair<std::string, std::string> generateKeyPair();
    static std::string encryptAuthCode(const std::string& auth_code, const std::string& public_key);
    static std::string decryptAuthCode(const std::string& auth_code, const std::string& private_key);
    static std::string encryptData(const std::string& data, const std::string& public_key);
    static std::string decryptData(const std::string& data, const std::string& private_key, size_t length);
    static std::string getLastError();

    // API Interface
    constexpr std::string getInterfaceName() const override;
    HttpEndPointData_t getHttpEndpointData() override;
};

#endif // AUTHENTICATION_H
