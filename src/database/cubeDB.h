#pragma once
#ifndef CUBEDB_H
#define CUBEDB_H
#ifndef DB_H
#include "db.h"
#endif
#ifndef API_I_H
#include "../api/api.h"
#endif
#include "nlohmann/json.hpp"
#include <condition_variable>
#include <functional>
#include <mutex>

class CubeDB : public AutoRegisterAPI<CubeDB> {
    static std::shared_ptr<CubeDatabaseManager> dbManager;
    static std::shared_ptr<BlobsManager> blobsManager;
    static bool isDBManagerSet;
    static bool isBlobsManagerSet;

public:
    CubeDB(std::shared_ptr<CubeDatabaseManager> dbManager, std::shared_ptr<BlobsManager> blobsManager);
    /**
     * @brief Construct a new CubeDB object. Must call setCubeDBManager and setBlobsManager before using this object.
     */
    CubeDB() {};
    ~CubeDB() {};
    static void setCubeDBManager(std::shared_ptr<CubeDatabaseManager> dbManager);
    static void setBlobsManager(std::shared_ptr<BlobsManager> blobsManager);
    static std::shared_ptr<CubeDatabaseManager> getDBManager();
    static std::shared_ptr<BlobsManager> getBlobsManager();
    // API Interface
    HttpEndPointData_t getHttpEndpointData() override;
    constexpr std::string getInterfaceName() const override;
};

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

static inline bool is_base64(unsigned char c)
{
    return (isalnum(c) || (c == '+') || (c == '/'));
}

// TODO: move these to the utils.h file
std::vector<unsigned char> base64_decode_cube(std::string const& encoded_string);
std::string base64_encode_cube(const std::vector<unsigned char>& bytes_to_encode);
std::string base64_encode_cube(const std::string& bytes_to_encode);

#endif // CUBEDB_H
