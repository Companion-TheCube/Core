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

EndPointData_t CubeDB::getEndpointData()
{
    // TODO: fill in the database endpoints
    EndPointData_t data;
    data.push_back({true, [&](std::string response, EndPointParams_t params){
        // TODO: test this function
        std::string blob = "none";
        std::string client_id = "none";
        std::string app_id = "none";
        std::string auth_code = "none";
        for(auto param : params){
            if(param.first == "blob"){
                blob = param.second;
            }
            if(param.first == "client_id"){
                client_id = param.second;
            }
            if(param.first == "app_id"){
                app_id = param.second;
            }
            if(param.first == "auth_code"){
                auth_code = param.second;
            }
        }
        // check auth
        if(client_id != "none"){
            if(!CubeDB::DBManager()->getDatabase("auth")->rowExists("clients", "client_id = '" + client_id + "' AND auth_code = '" + auth_code + "'")){
                CubeLog::error("Client auth failed");
                return "Client auth failed";
            }
        }else if(app_id != "none"){
            if(!CubeDB::DBManager()->getDatabase("auth")->rowExists("apps", "app_id = '" + app_id + "' AND auth_code = '" + auth_code + "'")){
                CubeLog::error("App auth failed");
                return "App auth failed";
            }
        }else{
            CubeLog::error("No client_id or app_id provided");
            return "No client_id or app_id provided";
        }
        if(client_id != "none"){
            CubeDB::DBManager()->getDatabase("blobs")->insertData("client_blobs", {"blob", "owner_client_id"}, {blob, client_id});
        }else if(app_id != "none"){
            CubeDB::DBManager()->getDatabase("blobs")->insertData("app_blobs", {"blob", "owner_app_id"}, {blob, app_id});
        }else{
            CubeLog::error("No client_id or app_id provided");
        }
        CubeLog::info("Blob saved");
        return "saveBlob called";
    }});
    return data;
}

std::vector<std::string> CubeDB::getEndpointNames()
{
    std::vector<std::string> names;
    names.push_back("saveBlob");
    return names;
}

std::string CubeDB::getIntefaceName() const
{
    return "CubeDB";
}