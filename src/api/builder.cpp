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
        auto endpointNames = i_face_obj->getEndpointNames();
        auto endpointData = i_face_obj->getEndpointData();
        if(endpointNames.size() != endpointData.size()){
            CubeLog::error("Error: Size of endpoint names and data do not match for interface: " + name + ". Skipping.");
            continue;
        }
        for(int i = 0; i < endpointNames.size(); i++){
            CubeLog::log("Adding endpoint: " + endpointNames[i] + " at " + name + "/" + endpointNames[i], true);
            std::string endpointPath = name + "-" + endpointNames.at(i);
            this->api->addEndpoint(endpointPath, "/" + endpointPath, endpointData.at(i).first, endpointData.at(i).second);
        }
    }
    CubeLog::log("API Builder finished build process.", true);
    this->api->start();
}
