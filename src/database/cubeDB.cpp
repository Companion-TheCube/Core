#include "cubeDB.h"

std::shared_ptr<CubeDatabaseManager> CubeDB::dbManager = nullptr;
std::shared_ptr<BlobsManager> CubeDB::blobsManager = nullptr;
bool CubeDB::isDBManagerSet = false;
bool CubeDB::isBlobsManagerSet = false;

CubeDB::CubeDB(std::shared_ptr<CubeDatabaseManager> dbManager, std::shared_ptr<BlobsManager> blobsManager)
{
    this->dbManager = dbManager;
    this->blobsManager = blobsManager;
    CubeDB::isDBManagerSet = true;
    CubeDB::isBlobsManagerSet = true;
}

void CubeDB::setCubeDBManager(std::shared_ptr<CubeDatabaseManager> dbManager)
{
    CubeDB::dbManager = dbManager;
    CubeDB::isDBManagerSet = true;
}

void CubeDB::setBlobsManager(std::shared_ptr<BlobsManager> blobsManager)
{
    CubeDB::blobsManager = blobsManager;
    CubeDB::isBlobsManagerSet = true;
}

std::shared_ptr<CubeDatabaseManager> CubeDB::DBManager()
{
    if(!CubeDB::isDBManagerSet){
        throw std::runtime_error("CubeDBManager not set");
    }
    return CubeDB::dbManager;
}

std::shared_ptr<BlobsManager> CubeDB::GetBlobsManager()
{
    if(!CubeDB::isBlobsManagerSet){
        throw std::runtime_error("BlobsManager not set");
    }
    return CubeDB::blobsManager;
}