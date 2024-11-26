#ifndef BUILDER_H
#define BUILDER_H
#ifndef API_H
#include "api.h"
#endif
#include <any>
#include <filesystem>
#include <logger.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>

// #define ADD_INTERFACE(ClassName)            \
// private:                                    \
//     inline static APIRegistrar<ClassName> registrar{};

class API_Builder {
private:
    static std::shared_ptr<API> api;
    static std::unordered_map<std::string, I_API_Interface*> interface_objs;

public:
    // Pass in all the dependencies the API needs
    API_Builder(std::shared_ptr<API> api);
    ~API_Builder();
    void start();
    template <typename T>
    static void addInterfaceStatic(std::shared_ptr<T> interface_object)
    {
        CubeLog::info("Adding interface object: " + interface_object->getInterfaceName());
        // interface_objs[interface_object->getInterfaceName()] = interface_object.get();
        // TODO: this method gets called at unpredictable times, so we need to make sure we're
        // not adding the same interface object multiple times. We also need to make the API server
        // able to handle new interfaces being added at runtime.
    }
    template <typename T>
    void addInterface(std::shared_ptr<T> interface_object)
    {
        CubeLog::info("Adding interface object: " + interface_object->getInterfaceName());
        interface_objs[interface_object->getInterfaceName()] = interface_object.get();
    }
    // template <typename T>
    // std::shared_ptr<T> getInterface(const std::string& name)
    // {
    //     return std::any_cast<std::shared_ptr<T>>(interface_objs.at(name));
    // }
};



#endif // BUILDER_H