#include "cubeDB.h"

std::shared_ptr<CubeDatabaseManager> CubeDB::dbManager = nullptr;
std::shared_ptr<BlobsManager> CubeDB::blobsManager = nullptr;
bool CubeDB::isDBManagerSet = false;
bool CubeDB::isBlobsManagerSet = false;

/**
 * @brief Construct a new CubeDB object
 * 
 * @param dbManager Use std::make_shared<CubeDatabaseManager>() to create this object
 * @param blobsManager Use std::make_shared<BlobsManager>(dbManager, path) to create this object
 */
CubeDB::CubeDB(std::shared_ptr<CubeDatabaseManager> dbManager, std::shared_ptr<BlobsManager> blobsManager)
{
    this->dbManager = dbManager;
    this->blobsManager = blobsManager;
    CubeDB::isDBManagerSet = true;
    CubeDB::isBlobsManagerSet = true;
}

/**
 * @brief Set the CubeDBManager object
 * 
 * @param dbManager 
 */
void CubeDB::setCubeDBManager(std::shared_ptr<CubeDatabaseManager> dbManager)
{
    CubeDB::dbManager = dbManager;
    CubeDB::isDBManagerSet = true;
}

/**
 * @brief Set the Blobs Manager object
 * 
 * @param blobsManager 
 */
void CubeDB::setBlobsManager(std::shared_ptr<BlobsManager> blobsManager)
{
    CubeDB::blobsManager = blobsManager;
    CubeDB::isBlobsManagerSet = true;
}

/**
 * @brief Get the CubeDBManager object
 * 
 * @return std::shared_ptr<CubeDatabaseManager> 
 */
std::shared_ptr<CubeDatabaseManager> CubeDB::DBManager()
{
    if(!CubeDB::isDBManagerSet){
        throw std::runtime_error("CubeDBManager not set");
    }
    return CubeDB::dbManager;
}

/**
 * @brief Get the Blobs Manager object
 * 
 * @return std::shared_ptr<BlobsManager> 
 */
std::shared_ptr<BlobsManager> CubeDB::GetBlobsManager()
{
    if(!CubeDB::isBlobsManagerSet){
        throw std::runtime_error("BlobsManager not set");
    }
    return CubeDB::blobsManager;
}

/**
 * @brief Get the Endpoint Data object
 * 
 * @return EndPointData_t 
 */
EndPointData_t CubeDB::getEndpointData()
{
    // TODO: fill in the database endpoints
    EndPointData_t data;
    data.push_back({false, [&](std::string response, EndPointParams_t params){
        // TODO: test this function
        std::string blob = "none";
        std::string client_id = "none";
        std::string app_id = "none";
        for(auto param : params){
            if(param.first == "blob"){ // TODO: the params (response, params) sent to this lambda will be combined and will include the body of the request which will be the blob
                blob = param.second;
            }
            if(param.first == "client_id"){
                client_id = param.second;
            }
            if(param.first == "app_id"){
                app_id = param.second;
            }
        }
        if(client_id != "none"){
            CubeDB::DBManager()->getDatabase("blobs")->insertData("client_blobs", {"blob", "owner_client_id"}, {blob, client_id});
            CubeLog::info("Blob saved");
        }else if(app_id != "none"){
            CubeDB::DBManager()->getDatabase("blobs")->insertData("app_blobs", {"blob", "owner_app_id"}, {blob, app_id});
            CubeLog::info("Blob saved");
        }else{
            CubeLog::error("No client_id or app_id provided");
        }
        return "saveBlob called";
    }});
    return data;
}

/**
 * @brief Get the Endpoint Names object
 * 
 * @return std::vector<std::string> 
 */
std::vector<std::string> CubeDB::getEndpointNames()
{
    std::vector<std::string> names;
    names.push_back("saveBlob");
    return names;
}

/**
 * @brief Get the Inteface Name object
 * 
 * @return std::string 
 */
std::string CubeDB::getIntefaceName() const
{
    return "CubeDB";
}