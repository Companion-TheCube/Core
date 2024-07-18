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
 * @return HttpEndPointData_t 
 */
HttpEndPointData_t CubeDB::getHttpEndpointData()
{
    // TODO: fill in the database endpoints
    HttpEndPointDataSinglet_t saveBlob = {PRIVATE_ENDPOINT | POST_ENDPOINT, [&](const httplib::Request &req, httplib::Response &res){
        if(!req.has_header("Content-Type") || req.get_header_value("Content-Type") != "application/json"){
            CubeLog::error("saveBlob called: failed to save blob, invalid Content-Type header.");
            nlohmann::json j;
            j["success"] = false;
            j["message"] = "saveBlob called: failed to save blob, invalid Content-Type header.";
            res.set_content(j.dump(), "application/json");
            return j.dump();
        }
        char buffer[65535];
        std::string stringBlob = "none";
        // 1MB blob size limit
        char* blob = new char[1024*1024];
        bool binaryBlob = false;
        size_t blobSize = 0;
        std::string client_id = "none";
        std::string app_id = "none";
        try{
            nlohmann::json received = nlohmann::json::parse(req.body);
            if(received.contains("stringBlob")){
                stringBlob = received["stringBlob"];
                blobSize = stringBlob.size();
            }else if(received.contains("blob")){
                binaryBlob = true;
                // blob is a base64 encoded binary blob
                stringBlob = received["blob"];
                // decode the base64 string
                std::vector<unsigned char> decoded = base64_decode(stringBlob);
                if(decoded.size() == 0){
                    CubeLog::error("saveBlob called: failed to save blob, base64 decoding failed.");
                    nlohmann::json j;
                    j["success"] = false;
                    j["message"] = "saveBlob called: failed to save blob, base64 decoding failed.";
                    res.set_content(j.dump(), "application/json");
                    delete[] blob;
                    return j.dump();
                }
                blobSize = decoded.size();
                if(blobSize > 1024*1024){
                    CubeLog::error("saveBlob called: failed to save blob, blob size exceeds 1MB limit.");
                    nlohmann::json j;
                    j["success"] = false;
                    j["message"] = "saveBlob called: failed to save blob, blob size exceeds 1MB limit.";
                    res.set_content(j.dump(), "application/json");
                    delete[] blob;
                    return j.dump();
                }
                size_t decodedSize = 0;
                for(auto c : decoded){
                    blob[decodedSize++] = c;
                }
            }
            if(received.contains("client_id")){
                client_id = received["client_id"];
            }
            if(received.contains("app_id")){
                app_id = received["app_id"];
            }
        }catch(std::exception &e){
            CubeLog::error("saveBlob called: failed to save blob,"  + std::string(e.what()));
            nlohmann::json j;
            j["success"] = false;
            j["message"] = "saveBlob called: failed to save blob, " + std::string(e.what());
            res.set_content(j.dump(), "application/json");
            delete[] blob;
            return j.dump();
        }
        nlohmann::json j;
        long blob_id = -2;
        std::mutex m;
        std::condition_variable cv;
        bool wait = true;
        std::function<void()> fn;
        if(client_id != "none"){
            fn = [&](){
                blob_id = CubeDB::getDBManager()->getDatabase("blobs")->insertData("client_blobs", {"id", "blob", "blob_size", "owner_client_id"}, {"",binaryBlob?std::string(blob,blobSize):stringBlob, std::to_string(blobSize), client_id});
                CubeLog::info("Blob saved");
                std::lock_guard<std::mutex> lock(m);
                wait = false;
                cv.notify_one();
            };
        }else if(app_id != "none"){
            fn = [&](){
                blob_id = CubeDB::getDBManager()->getDatabase("blobs")->insertData("app_blobs", {"id","blob", "blob_size", "owner_app_id"}, {"",binaryBlob?std::string(blob,blobSize):stringBlob, std::to_string(blobSize), app_id});
                CubeLog::info("Blob saved");
                std::lock_guard<std::mutex> lock(m);
                wait = false;
                cv.notify_one();
            };
        }else{
            CubeLog::error("No client_id or app_id provided");
            j["success"] = false;
            j["message"] = "saveBlob called: failed to save blob, no client_id or app_id provided.";
            res.set_content(j.dump(), "application/json");
            delete[] blob;
            return j.dump();
        }
        CubeDB::getDBManager()->addDbTask(fn); // ensures that the database operation is performed on the database thread.
        // these next two lines are necessary to ensure that the response is not sent until the database operation is complete and the blob_id is set.
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&]{return !wait;});
        j["success"] = true;
        j["message"] = "saveBlob called and completed.";
        if(blob_id < 0){
            j["success"] = false;
            j["message"] = "saveBlob called: failed to save blob, database error:" + CubeDB::getDBManager()->getDatabase("blobs")->getLastError();
        }
        j["blob_id"] = blob_id;
        res.set_content(j.dump(), "application/json");
        return j.dump();
    }};
    HttpEndPointDataSinglet_t insertDataTest = {PUBLIC_ENDPOINT | GET_ENDPOINT, [&](const httplib::Request &req, httplib::Response &res){
        // first we create a buffer to hold the response
        char* ret = new char[65535];
        int size = 0;
        // then we create a lambda function to be executed on the database thread
        auto fn = [&](){
            // here we perform the database operation(s)
            bool success1 = (-1 < CubeDB::getDBManager()->getDatabase("auth")->insertData(DB_NS::TableNames::CLIENTS, {"initial_code", "auth_code", "client_id", "role"}, {"1234", "5678", "test", "1"}));
            if(!success1){
                CubeLog::error(CubeDB::getDBManager()->getDatabase("auth")->getLastError());
            }
            bool success2 = (-1 < CubeDB::getDBManager()->getDatabase("auth")->insertData(DB_NS::TableNames::APPS, {"auth_code", "public_key", "private_key", "app_id", "role"}, {"5678", "public", "private", "test", "1"}));
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
    }};
    // TODO: endpoints to write:
    // retrieveBlobBinary - private, get - get a binary blob from the database, returns base64 encoded string
    // retrieveBlobString - private, get - get a string blob from the database, returns string
    // retrieveData - private, get - get data from the database, returns json
    // 
    HttpEndPointData_t data;
    data.push_back(saveBlob);
    data.push_back(insertDataTest);
    return data;
}

/**
 * @brief Get the Endpoint Names object
 * 
 * @return std::vector<std::pair<std::string, std::vector<std::string>>> 
 * @details This function returns a vector of pairs. Each pair contains a string and a vector of strings. The string is the name of the endpoint and the vector of strings are the parameters that the endpoint accepts.
 */
std::vector<std::pair<std::string,std::vector<std::string>>> CubeDB::getHttpEndpointNamesAndParams()
{
    std::vector<std::string> names;
    std::vector<std::string> params;
    std::pair<std::string,std::vector<std::string>> saveBlob;
    saveBlob.first = "saveBlob";
    saveBlob.second = {};
    std::pair<std::string,std::vector<std::string>> insertData;
    insertData.first = "insertData";
    insertData.second = {};
    std::vector<std::pair<std::string,std::vector<std::string>>> namesAndParams;
    namesAndParams.push_back(saveBlob);
    namesAndParams.push_back(insertData);
    return namesAndParams;
}

/**
 * @brief Get the Inteface Name for the database endpoints
 * 
 * @return std::string 
 */
std::string CubeDB::getIntefaceName() const
{
    return "CubeDB";
}

/**
 * @brief Base64 decode a string. Based on code from https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp
 * 
 * @param encoded_string 
 * @return std::vector<unsigned char> 
 */
std::vector<unsigned char> base64_decode(const std::string& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::vector<unsigned char> ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i == 4) {
      for (i = 0; i < 4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = ( char_array_4[0] << 2       ) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret.push_back(char_array_3[i]);
      i = 0;
    }
  }

  if (i) {
    for (j = 0; j < i; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = ( char_array_4[0] << 2       ) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

    for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
  }

  return ret;
}

/**
 * @brief Base64 encode a string. Based on code from https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp
 * 
 * @param bytes_to_encode
 * @return std::string 
 */
std::string base64_encode(const std::vector<unsigned char>& bytes_to_encode) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  for (unsigned char byte : bytes_to_encode) {
    char_array_3[i++] = byte;
    if (i == 3) {
      char_array_4[0] = ( char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] =   char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = ( char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';
  }

  return ret;
}