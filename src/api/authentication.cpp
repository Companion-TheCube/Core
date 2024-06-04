#include "authentication.h"

std::string KeyGenerator(size_t length)
{
    std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string result;
    result.resize(length);
    for (size_t i = 0; i < length; i++) {
        result[i] = charset[rand() % charset.length()];
    }
    return result;
}

std::string Code6Generator()
{
    std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890123456789+=&%$#@!";
    std::string result;
    result.resize(6);
    for (size_t i = 0; i < 6; i++) {
        result[i] = charset[rand() % charset.length()];
    }
    return result;
}

CubeAuth::CubeAuth()
{
    CubeLog::log("Authentication module starting...", true);
}

CubeAuth::~CubeAuth()
{
    CubeLog::log("Authentication module stopping...", true);
}