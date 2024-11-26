#include "builder.h"

std::shared_ptr<API> API_Builder::api = nullptr;
std::unordered_map<std::string, I_API_Interface*> API_Builder::interface_objs;

/**
 * @brief Construct a new api builder::api builder object
 *
 * @param api - An instance of the API object as a shared pointer
 */
API_Builder::API_Builder(std::shared_ptr<API> api)
{
    this->api = api;
}

/**
 * @brief Destroy the API Builder::API Builder object
 *
 */
API_Builder::~API_Builder()
{
    CubeLog::info("API Builder destroyed");
}

/**
 * @brief Start the API builder process. This will build all the endpoints and start the API.
 *
 */
void API_Builder::start()
{
    // Start the API
    CubeLog::info("API Builder begin build process.");
    // Using all the components that are passed in, build the API endpoints
    for (auto &[name, i_face_obj] : this->interface_objs) {
        CubeLog::info("Building interface object: " + name);
        auto endpointData = i_face_obj->getHttpEndpointData();
        for (size_t i = 0; i < endpointData.size(); i++) {
            CubeLog::info("Adding endpoint: " + std::get<2>(endpointData.at(i)) + " at " + name + "/" + std::get<2>(endpointData.at(i)));
            std::string endpointPath = name + "-" + std::get<2>(endpointData.at(i));
            this->api->addEndpoint(endpointPath, "/" + endpointPath, std::get<0>(endpointData.at(i)), std::get<1>(endpointData.at(i)));
        }
    }
    // create an endpoint that will return the list of all endpoints as json, including the parameters and whether or not they are public
    CubeLog::info("Adding endpoint: getEndpoints at /getEndpoints");
    this->api->addEndpoint("getEndpoints", "/getEndpoints", PUBLIC_ENDPOINT | GET_ENDPOINT, [&](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j;
        for (auto &[name, i_face_obj] : this->interface_objs) {
            auto endpointData = i_face_obj->getHttpEndpointData();
            for (size_t i = 0; i < endpointData.size(); i++) {
                nlohmann::json endpoint_json;
                endpoint_json["name"] = std::get<2>(endpointData.at(i));
                endpoint_json["params"] = std::get<3>(endpointData.at(i));
                endpoint_json["description"] = std::get<4>(endpointData.at(i));
                endpoint_json["public"] = (std::get<0>(endpointData.at(i)) & PUBLIC_ENDPOINT) == PUBLIC_ENDPOINT ? "true" : "false";
                endpoint_json["endpoint_type"] = (std::get<0>(endpointData.at(i)) & GET_ENDPOINT) == GET_ENDPOINT ? "GET" : "POST";
                j[name].push_back(endpoint_json);
            }
        }
        res.set_content(j.dump(), "application/json");
        return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, j.dump());
    });

    // add endpoint that will return the cube.socket path
    CubeLog::info("Adding endpoint: getCubeSocketPath at /getCubeSocketPath");
    this->api->addEndpoint("getCubeSocketPath", "/getCubeSocketPath", PUBLIC_ENDPOINT | GET_ENDPOINT, [&](const httplib::Request& req, httplib::Response& res) {
        std::filesystem::path path = std::filesystem::current_path();
        // make a json object that contains the cube.socket path
        nlohmann::json j;
        j["cube_socket_path"] = path.string() + "/" + "cube.sock";
        res.set_content(j.dump(), "application/json");
        return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, j.dump());
    });
    // recursively search http folder for static files and add them to the server
    std::vector<std::filesystem::path> staticFiles;
    for (auto& p : std::filesystem::recursive_directory_iterator("http")) {
        if (p.is_regular_file()) {
            staticFiles.push_back(p.path());
            CubeLog::debug("Found static file: " + p.path().string());
        }
    }
    staticFiles.push_back(std::filesystem::path("http/"));
    // TODO: refactor to get rid of staticFiles vector. update: maybe not?
    for (auto file : staticFiles) {
        CubeLog::info("Adding static file: " + file.string());
        // strip off the ./http/ part of the path
        std::string endpointPath = file.string().substr(5);
        // replace all \ with /
        std::replace(endpointPath.begin(), endpointPath.end(), '\\', '/');
        CubeLog::debug("Endpoint path: " + endpointPath);
        std::string filePath = file.string();
        this->api->addEndpoint(filePath, "/" + endpointPath, PUBLIC_ENDPOINT | GET_ENDPOINT, [&, filePath](const httplib::Request& req, httplib::Response& res) {
            std::string l_path = filePath;
            if(l_path.empty() || l_path == "http/" || l_path == "http"){
                l_path = "http/index.html";
            }
            if (!std::filesystem::exists(l_path)) {
                CubeLog::error("File not found: " + l_path);
                // return std::string("File not found: " + filePath);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NOT_FOUND, "File not found: " + l_path);
            } else if (std::filesystem::is_directory(l_path)) {
                CubeLog::error("File is a directory: " + l_path);
                // return std::string("File is a directory: " + filePath);
                EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "File is a directory: " + l_path);
            } else
                CubeLog::info("Sending file: " + l_path);
            std::ifstream fileStream(l_path, std::ios::binary | std::ios::ate);
            if (!fileStream.is_open()) {
                CubeLog::error("Error opening file: " + l_path);
                // return std::string("Error opening file: " + filePath);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Error opening file: " + l_path);
            }
            std::streamsize fileSize = fileStream.tellg();
            fileStream.seekg(0, std::ios::beg);
            std::vector<char> buffer(fileSize);
            if (!fileStream.read(buffer.data(), fileSize)) {
                CubeLog::error("Error reading file: " + l_path);
                // return std::string("Error reading file: " + filePath);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Error reading file: " + l_path);
            }
            if (fileStream.fail() || fileStream.bad()) {
                CubeLog::error("Error reading file: " + l_path);
                // return std::string("Error reading file: " + filePath);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Error reading file: " + l_path);
            }
            fileStream.close();
            // set the content type based on the file extension
            std::string contentType = "text/plain";
            if (l_path.find(".html") != std::string::npos) {
                contentType = "text/html";
            } else if (l_path.find(".css") != std::string::npos) {
                contentType = "text/css";
            } else if (l_path.find(".js") != std::string::npos) {
                contentType = "text/javascript";
            } else if (l_path.find(".png") != std::string::npos) {
                contentType = "image/png";
            } else if (l_path.find(".jpg") != std::string::npos) {
                contentType = "image/jpeg";
            } else if (l_path.find(".ico") != std::string::npos) {
                contentType = "image/x-icon";
            } else if (l_path.find(".json") != std::string::npos) {
                contentType = "application/json";
            } else if (l_path.find(".txt") != std::string::npos) {
                contentType = "text/plain";
            } else if (l_path.find(".svg") != std::string::npos) {
                contentType = "image/svg+xml";
            } else if (l_path.find(".pdf") != std::string::npos) {
                contentType = "application/pdf";
            } else if (l_path.find(".mp4") != std::string::npos) {
                contentType = "video/mp4";
            } else if (l_path.find(".mp3") != std::string::npos) {
                contentType = "audio/mp3";
            } else if (l_path.find(".wav") != std::string::npos) {
                contentType = "audio/wave";
            } else if (l_path.find(".webm") != std::string::npos) {
                contentType = "video/webm";
            } else if (l_path.find(".webp") != std::string::npos) {
                contentType = "image/webp";
            } else if (l_path.find(".woff") != std::string::npos) {
                contentType = "font/woff";
            } else if (l_path.find(".woff2") != std::string::npos) {
                contentType = "font/woff2";
            } else if (l_path.find(".otf") != std::string::npos) {
                contentType = "font/otf";
            } else if (l_path.find(".ttf") != std::string::npos) {
                contentType = "font/ttf";
            } else if (l_path.find(".csv") != std::string::npos) {
                contentType = "text/csv";
            } else if (l_path.find(".vtt") != std::string::npos) {
                contentType = "text/vtt";
            } else if (l_path.find(".7z") != std::string::npos) {
                contentType = "application/x-7z-compressed";
            } else if (l_path.find(".atom") != std::string::npos) {
                contentType = "application/atom+xml";
            } else if (l_path.find(".apng") != std::string::npos) {
                contentType = "image/apng";
            } else if (l_path.find(".avif") != std::string::npos) {
                contentType = "image/avif";
            } else if (l_path.find(".bmp") != std::string::npos) {
                contentType = "image/bmp";
            } else if (l_path.find(".gif") != std::string::npos) {
                contentType = "image/gif";
            } else if (l_path.find(".jpeg") != std::string::npos) {
                contentType = "image/jpeg";
            } else if (l_path.find(".rss") != std::string::npos) {
                contentType = "application/rss+xml";
            } else if (l_path.find(".tif") != std::string::npos) {
                contentType = "image/tiff";
            } else if (l_path.find(".tiff") != std::string::npos) {
                contentType = "image/tiff";
            } else if (l_path.find(".tar") != std::string::npos) {
                contentType = "application/x-tar";
            } else if (l_path.find(".xhtml") != std::string::npos) {
                contentType = "application/xhtml+xml";
            } else if (l_path.find(".xht") != std::string::npos) {
                contentType = "application/xhtml+xml";
            } else if (l_path.find(".xslt") != std::string::npos) {
                contentType = "application/xslt+xml";
            } else if (l_path.find(".xml") != std::string::npos) {
                contentType = "application/xml";
            } else if (l_path.find(".gz") != std::string::npos) {
                contentType = "application/gzip";
            } else if (l_path.find(".zip") != std::string::npos) {
                contentType = "application/zip";
            } else if (l_path.find(".wasm") != std::string::npos) {
                contentType = "application/wasm";
            } else if (l_path.find(".blob") != std::string::npos) {
                contentType = "application/octet-stream";
            } else {
                CubeLog::warning("Unknown file type: " + l_path);
                contentType = "application/octet-stream";
            }
            char* fileData = new char[fileSize];
            memcpy(fileData, buffer.data(), fileSize);
            res.set_content(fileData, fileSize, contentType);
            delete[] fileData;
            CubeLog::debug("File sent: " + l_path + " (" + std::to_string(fileSize) + " bytes)");
            // return std::string("");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        });
    }

    CubeLog::info("API Builder finished build process.");
    this->api->start();
}
