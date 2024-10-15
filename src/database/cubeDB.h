#pragma once
#ifndef CUBEDB_H
#define CUBEDB_H
#ifndef DB_H
#include "db.h"
#endif
#ifndef API_I_H
#include "../api/api_i.h"
#endif
#include <functional>
#include <mutex>
#include <condition_variable>

class CubeDB : public I_API_Interface{
    static std::shared_ptr<CubeDatabaseManager> dbManager;
    static std::shared_ptr<BlobsManager> blobsManager;
    static bool isDBManagerSet;
    static bool isBlobsManagerSet;
public:
    CubeDB(std::shared_ptr<CubeDatabaseManager> dbManager, std::shared_ptr<BlobsManager> blobsManager);
    /**
     * @brief Construct a new CubeDB object. Must call setCubeDBManager and setBlobsManager before using this object. 
     */
    CubeDB(){};
    ~CubeDB(){};
    static void setCubeDBManager(std::shared_ptr<CubeDatabaseManager> dbManager);
    static void setBlobsManager(std::shared_ptr<BlobsManager> blobsManager);
    static std::shared_ptr<CubeDatabaseManager> getDBManager();
    static std::shared_ptr<BlobsManager> getBlobsManager();
    // API Interface
    HttpEndPointData_t getHttpEndpointData() override;
    // std::vector<std::pair<std::string,std::vector<std::string>>> getHttpEndpointNamesAndParams() override;
    std::string getIntefaceName() const override;
};


static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}
std::vector<unsigned char> base64_decode_cube(std::string const& encoded_string);
std::string base64_encode_cube(const std::vector<unsigned char>& bytes_to_encode);

#endif// CUBEDB_H
