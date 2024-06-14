#include <httplib.h>
#include <logger.h>
#include <utils.h>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>

class DockerAPI {
private:
    httplib::Client client;
    std::string base_url;
public:
    DockerAPI(const std::string& base_url);
    DockerAPI();
    std::string listContainers();
    std::string listImages();
    std::string startContainer(const std::string& container_id);
    std::string stopContainer(const std::string& container_id);
    std::string inspectContainer(const std::string& container_id);
    bool isContainerRunning(const std::string& container_id);
    std::vector<std::string> getContainers();
};
