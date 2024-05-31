#pragma once
#include <logger.h>
#include "api.h"
#include <any>
#include <unordered_map>
#include <memory>

class API_Builder {
private:
    CubeLog *logger;
    std::shared_ptr<API> api;
    std::unordered_map<std::string, I_API_Interface*> interface_objs;
public:
    // Pass in all the dependencies the API needs
    API_Builder(CubeLog *logger, std::shared_ptr<API> api);
    ~API_Builder();
    void start();
    template <typename T>
    void addInterface(std::shared_ptr<T> interface_object){
        this->logger->log("Adding interface object: " + interface_object->getIntefaceName(), true);
        this->interface_objs[interface_object->getIntefaceName()] = interface_object.get();
    }
    template <typename T>
    std::shared_ptr<T> getInterface(const std::string& name){
        return std::any_cast<std::shared_ptr<T>>(interface_objs.at(name));
    }
};

