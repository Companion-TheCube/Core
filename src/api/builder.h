#ifndef BUILDER_H
#define BUILDER_H
#ifndef API_H
#include "api.h"
#endif
#include <any>
#include <logger.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <filesystem>

class API_Builder {
private:
    std::shared_ptr<API> api;
    std::unordered_map<std::string, I_API_Interface*> interface_objs;

public:
    // Pass in all the dependencies the API needs
    API_Builder(std::shared_ptr<API> api);
    ~API_Builder();
    void start();
    template <typename T>
    void addInterface(std::shared_ptr<T> interface_object)
    {
        CubeLog::info("Adding interface object: " + interface_object->getIntefaceName());
        this->interface_objs[interface_object->getIntefaceName()] = interface_object.get();
    }
    template <typename T>
    std::shared_ptr<T> getInterface(const std::string& name)
    {
        return std::any_cast<std::shared_ptr<T>>(interface_objs.at(name));
    }
};


#endif// BUILDER_H