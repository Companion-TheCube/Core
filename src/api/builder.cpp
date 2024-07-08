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
    this->api->addEndpoint("getEndpoints", "/getEndpoints", true, [&](const httplib::Request &req){
        nlohmann::json j;
        for(auto i_face: this->interface_objs){
            std::string name = i_face.first;
            I_API_Interface *i_face_obj = i_face.second;
            auto endpointNames = i_face_obj->getEndpointNamesAndParams();
            for(size_t i = 0; i < endpointNames.size(); i++){
                nlohmann::json endpoint_json;
                endpoint_json["name"] = endpointNames.at(i).first;
                endpoint_json["params"] = endpointNames.at(i).second;
                endpoint_json["public"] = i_face_obj->getEndpointData().at(i).first;
                j[name].push_back(endpoint_json);
            }
        }
        return j.dump();
    });
    // TODO: add endpoints to authorize clients. this may be built into authentication.cpp file.
    CubeLog::log("API Builder finished build process.", true);
    this->api->start();
}
