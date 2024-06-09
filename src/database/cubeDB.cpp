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
std::shared_ptr<CubeDatabaseManager> CubeDB::getDBManager()
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
std::shared_ptr<BlobsManager> CubeDB::getBlobsManager()
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
    data.push_back({false, [&](const httplib::Request &req){
        std::string blob = "none";
        std::string client_id = "none";
        std::string app_id = "none";
        for(auto param : req.params){
            if(param.first == "blob"){
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
            CubeDB::getDBManager()->getDatabase("blobs")->insertData("client_blobs", {"blob", "owner_client_id"}, {blob, client_id});
            CubeLog::info("Blob saved");
        }else if(app_id != "none"){
            CubeDB::getDBManager()->getDatabase("blobs")->insertData("app_blobs", {"blob", "owner_app_id"}, {blob, app_id});
            CubeLog::info("Blob saved");
        }else{
            CubeLog::error("No client_id or app_id provided");
        }
        return "saveBlob called";
    }});
    data.push_back({true, [&](const httplib::Request &req){
        // first we create a buffer to hold the response
        char* ret = new char[65535];
        int size = 0;
        // then we create a lambda function to be executed on the database thread
        auto fn = [&](){
            // here we perform the database operation(s)
            bool success1 = CubeDB::getDBManager()->getDatabase("auth")->insertData(DB_NS::TableNames::CLIENTS, {"initial_code", "auth_code", "client_id", "role"}, {"1234", "5678", "test", "1"});
            if(!success1){
                CubeLog::error(CubeDB::getDBManager()->getDatabase("auth")->getLastError());
            }
            bool success2 = CubeDB::getDBManager()->getDatabase("auth")->insertData(DB_NS::TableNames::APPS, {"auth_code", "public_key", "private_key", "app_id", "role"}, {"5678", "public", "private", "test", "1"});
            if(!success2){
                CubeLog::error(CubeDB::getDBManager()->getDatabase("auth")->getLastError());
            }
            // If the operation returns data, we copy it to the response buffer
            std::string done = success1&&success2?"Data inserted":"Insertion failed";
            size = done.size();
            done.copy(ret, size);
        };
        // With the lambda function created, we add it to the database thread. It will be executed as soon as possible.
        CubeDB::getDBManager()->addDbTask(fn);
        // We wait for the operation to complete in order to return the response. This is a blocking call and is only 
        // necessary for DB operations that return data. 
        while(size == 0){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        // We copy the response buffer to a string and delete the buffer
        std::string retStr(ret, size);
        delete[] ret;
        // We return the response string
        return retStr;
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
    names.push_back("insertData");
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