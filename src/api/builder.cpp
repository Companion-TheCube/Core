#include "builder.h"

API_Builder::API_Builder(std::shared_ptr<API> api){
    this->api = api;
}

API_Builder::~API_Builder(){
    CubeLog::log("API Builder destroyed", true);
}

void API_Builder::start(){
    // Start the API
    CubeLog::log("API Builder begin build process.", true);
    // Using all the components that are passed in, build the API endpoints
    for(auto i_face: this->interface_objs){
        std::string name = i_face.first;
        I_API_Interface *i_face_obj = i_face.second;
        CubeLog::log("Building interface object: " + name, true);
        auto endpointNames = i_face_obj->getEndpointNamesAndParams();
        auto endpointData = i_face_obj->getEndpointData();
        if(endpointNames.size() != endpointData.size()){
            CubeLog::error("Error: Size of endpoint names and data do not match for interface: " + name + ". Skipping.");
            continue;
        }
        for(int i = 0; i < endpointNames.size(); i++){
            CubeLog::log("Adding endpoint: " + endpointNames.at(i).first + " at " + name + "/" + endpointNames.at(i).first, true);
            std::string endpointPath = name + "-" + endpointNames.at(i).first;
            this->api->addEndpoint(endpointPath, "/" + endpointPath, endpointData.at(i).first, endpointData.at(i).second);
        }
    }
    // create an endpoint that will return the list of all endpoints as json, including the parameters and whether or not they are public
    CubeLog::log("Adding endpoint: getEndpoints at /getEndpoints", true);
    this->api->addEndpoint("getEndpoints", "/getEndpoints", PUBLIC_ENDPOINT | GET_ENDPOINT, [&](const httplib::Request &req, httplib::Response &res){
        nlohmann::json j;
        for(auto i_face: this->interface_objs){
            std::string name = i_face.first;
            I_API_Interface *i_face_obj = i_face.second;
            auto endpointNames = i_face_obj->getEndpointNamesAndParams();
            for(size_t i = 0; i < endpointNames.size(); i++){
                nlohmann::json endpoint_json;
                endpoint_json["name"] = endpointNames.at(i).first;
                endpoint_json["params"] = endpointNames.at(i).second;
                endpoint_json["public"] = i_face_obj->getEndpointData().at(i).first & PUBLIC_ENDPOINT == PUBLIC_ENDPOINT ? "true" : "false";
                endpoint_json["endpoint_type"] = i_face_obj->getEndpointData().at(i).first & GET_ENDPOINT == GET_ENDPOINT ? "GET" : "POST";
                j[name].push_back(endpoint_json);
            }
        }
        return j.dump();
    });
    
    // TODO: add endpoints to authorize clients. this may be built into authentication.cpp file.

    // recursively search http folder for static files and add them to the server
    std::vector<std::filesystem::path> staticFiles;
    for(auto &p: std::filesystem::recursive_directory_iterator("http")){
        if(p.is_regular_file()){
            staticFiles.push_back(p.path());
            CubeLog::debug("Found static file: " + p.path().string());
        }
    }
    for(auto file: staticFiles){
        CubeLog::info("Adding static file: " + file.string());
        // strip off the ./http/ part of the path
        std::string endpointPath = file.string().substr(5);
        // replace all \ with /
        std::replace(endpointPath.begin(), endpointPath.end(), '\\', '/');
        CubeLog::debug("Endpoint path: " + endpointPath);
        std::string filePath = file.string();
        this->api->addEndpoint(filePath, "/" + endpointPath, PUBLIC_ENDPOINT | GET_ENDPOINT, [&, filePath](const httplib::Request &req, httplib::Response &res){
            if(!std::filesystem::exists(filePath)) {
                CubeLog::error("File not found: " + filePath);
                return std::string("File not found: " + filePath);
            }else if(std::filesystem::is_directory(filePath)) {
                CubeLog::error("File is a directory: " + filePath);
                return std::string("File is a directory: " + filePath);
            }else CubeLog::info("Sending file: " + filePath);
            std::ifstream fileStream(filePath, std::ios::binary | std::ios::ate);
            if(!fileStream.is_open()) {
                CubeLog::error("Error opening file: " + filePath);
                return std::string("Error opening file: " + filePath);
            }
            std::streamsize fileSize = fileStream.tellg();
            fileStream.seekg(0, std::ios::beg);
            std::vector<char> buffer(fileSize);
            if(!fileStream.read(buffer.data(), fileSize)) {
                CubeLog::error("Error reading file: " + filePath);
                return std::string("Error reading file: " + filePath);
            }
            if(fileStream.fail() || fileStream.bad()){
                CubeLog::error("Error reading file: " + filePath);
                return std::string("Error reading file: " + filePath);
            }
            fileStream.close();
            // set the content type based on the file extension
            std::string contentType = "text/plain";
            if(filePath.find(".html") != std::string::npos){
                contentType = "text/html";
            }else if(filePath.find(".css") != std::string::npos){
                contentType = "text/css";
            }else if(filePath.find(".js") != std::string::npos){
                contentType = "text/javascript";
            }else if(filePath.find(".png") != std::string::npos){
                contentType = "image/png";
            }else if(filePath.find(".jpg") != std::string::npos){
                contentType = "image/jpeg";
            }else if(filePath.find(".ico") != std::string::npos){
                contentType = "image/x-icon";
            }else if(filePath.find(".json") != std::string::npos){
                contentType = "application/json";
            }else if(filePath.find(".txt") != std::string::npos){
                contentType = "text/plain";
            }else if(filePath.find(".svg") != std::string::npos){
                contentType = "image/svg+xml";
            }else if(filePath.find(".pdf") != std::string::npos){
                contentType = "application/pdf";
            }else if(filePath.find(".mp4") != std::string::npos){
                contentType = "video/mp4";
            }else if(filePath.find(".mp3") != std::string::npos){
                contentType = "audio/mp3";
            }else if(filePath.find(".wav") != std::string::npos){
                contentType = "audio/wave";
            }else if(filePath.find(".webm") != std::string::npos){
                contentType = "video/webm";
            }else if(filePath.find(".webp") != std::string::npos){
                contentType = "image/webp";
            }else if(filePath.find(".woff") != std::string::npos){
                contentType = "font/woff";
            }else if(filePath.find(".woff2") != std::string::npos){
                contentType = "font/woff2";
            }else if(filePath.find(".otf") != std::string::npos){
                contentType = "font/otf";
            }else if(filePath.find(".ttf") != std::string::npos){
                contentType = "font/ttf";
            }else if(filePath.find(".csv") != std::string::npos){
                contentType = "text/csv";
            }else if(filePath.find(".vtt") != std::string::npos){
                contentType = "text/vtt";
            }else if(filePath.find(".7z") != std::string::npos){
                contentType = "application/x-7z-compressed";
            }else if(filePath.find(".atom") != std::string::npos){
                contentType = "application/atom+xml";
            }else if(filePath.find(".apng") != std::string::npos){
                contentType = "image/apng";
            }else if(filePath.find(".avif") != std::string::npos){
                contentType = "image/avif";
            }else if(filePath.find(".bmp") != std::string::npos){
                contentType = "image/bmp";
            }else if(filePath.find(".gif") != std::string::npos){
                contentType = "image/gif";
            }else if(filePath.find(".jpeg") != std::string::npos){
                contentType = "image/jpeg";
            }else if(filePath.find(".rss") != std::string::npos){
                contentType = "application/rss+xml";
            }else if(filePath.find(".tif") != std::string::npos){
                contentType = "image/tiff";
            }else if(filePath.find(".tiff") != std::string::npos){
                contentType = "image/tiff";
            }else if(filePath.find(".tar") != std::string::npos){
                contentType = "application/x-tar";
            }else if(filePath.find(".xhtml") != std::string::npos){
                contentType = "application/xhtml+xml";
            }else if(filePath.find(".xht") != std::string::npos){
                contentType = "application/xhtml+xml";
            }else if(filePath.find(".xslt") != std::string::npos){
                contentType = "application/xslt+xml";
            }else if(filePath.find(".xml") != std::string::npos){
                contentType = "application/xml";
            }else if(filePath.find(".gz") != std::string::npos){
                contentType = "application/gzip";
            }else if(filePath.find(".zip") != std::string::npos){
                contentType = "application/zip";
            }else if(filePath.find(".wasm") != std::string::npos){
                contentType = "application/wasm";
            }else if(filePath.find(".blob") != std::string::npos){
                contentType = "application/octet-stream";
            }else{
                CubeLog::warning("Unknown file type: " + filePath);
                contentType = "application/octet-stream";
            }
            char* fileData = new char[fileSize];
            memcpy(fileData, buffer.data(), fileSize);
            res.set_content(fileData, fileSize, contentType);
            delete[] fileData;
            CubeLog::debug("File sent: " + filePath + " (" + std::to_string(fileSize) + " bytes)");
            return std::string("");
        });
    }

    CubeLog::info("API Builder finished build process.");
    this->api->start();
}
