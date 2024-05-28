#include "builder.h"

API_Builder::API_Builder(CubeLog *logger, API *api){
    this->logger = logger;
    this->api = api;
}

API_Builder::~API_Builder(){
    this->logger->log("API Builder destroyed", true);
}

void API_Builder::start(){
    // Start the API
    this->logger->log("API Builder begin build process.", true);
    // Using all the components that are passed in, build the API endpoints
    for(auto i_face: this->interface_objs){
        std::string name = i_face.first;
        I_API_Interface *i_face_obj = i_face.second;
        this->logger->log("Building interface object: " + name, true);
        auto endpointNames = i_face_obj->getEndpointNames();
        auto endpointData = i_face_obj->getEndpointData();
        if(endpointNames.size() != endpointData.size()){
            this->logger->error("Error: Size of endpoint names and data do not match for interface: " + name + ". Skipping.");
            continue;
        }
        for(int i = 0; i < endpointNames.size(); i++){
            this->logger->log("Adding endpoint: " + endpointNames[i] + " at " + name + "/" + endpointNames[i], true);
            std::string endpointPath = name + "-" + endpointNames.at(i);
            this->api->addEndpoint(endpointPath, "/" + endpointPath, endpointData.at(i).first, endpointData.at(i).second);
        }
    }
    this->logger->log("API Builder finished build process.", true);
    this->api->start();
}
