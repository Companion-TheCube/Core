#pragma once
#include "db.h"
#include "../api/api_i.h"

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
    EndPointData_t getEndpointData() override;
    std::vector<std::string> getEndpointNames() override;
    std::string getIntefaceName() const override;
};