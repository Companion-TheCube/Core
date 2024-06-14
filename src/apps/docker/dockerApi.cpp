#include "dockerApi.h"

DockerAPI::DockerAPI(const std::string& base_url)
    : client(base_url)
    , base_url(base_url)
{

    std::string containers = this->listContainers();
    std::cout << "Containers: " << containers << std::endl;

    std::string images = this->listImages();
    std::cout << "Images: " << images << std::endl;
}

DockerAPI::DockerAPI()
    : client("http://127.0.0.1:2375")
    , base_url("http://127.0.0.1:2375")
{
    std::string containers = this->listContainers();
    std::cout << "Containers: " << containers << std::endl;

    std::string images = this->listImages();
    std::cout << "Images: " << images << std::endl;
}

std::string DockerAPI::listContainers()
{
    auto res = client.Get("/containers/json");
    if (res && res->status == 200) {
        return res->body;
    } else {
        return "Error: Unable to list containers";
    }
}

std::vector<std::string> DockerAPI::getContainers()
{
    std::string containers = this->listContainers();
    nlohmann::json containers_json = nlohmann::json::parse(containers);
    std::vector<std::string> container_ids;
    for (auto container : containers_json) {
        container_ids.push_back(container["Id"]);
    }
    return container_ids;
}

std::string DockerAPI::listImages()
{
    auto res = client.Get("/images/json");
    if (res && res->status == 200) {
        return res->body;
    } else {
        return "Error: Unable to list images";
    }
}

std::string DockerAPI::startContainer(const std::string& container_id)
{
    std::string endpoint = "/containers/" + container_id + "/start";
    auto res = client.Post(endpoint.c_str());
    if (res && res->status == 204) {
        return "Container started successfully";
    } else {
        return "Error: Unable to start container";
    }
}

std::string DockerAPI::stopContainer(const std::string& container_id)
{
    std::string endpoint = "/containers/" + container_id + "/stop";
    auto res = client.Post(endpoint.c_str());
    if (res && res->status == 204) {
        return "Container stopped successfully";
    } else {
        return "Error: Unable to stop container";
    }
}

std::string DockerAPI::inspectContainer(const std::string& container_id)
{
    std::string endpoint = "/containers/" + container_id + "/json";
    auto res = client.Get(endpoint.c_str());
    if (res && res->status == 200) {
        return res->body;
    } else {
        return "Error: Unable to inspect container";
    }
}

bool DockerAPI::isContainerRunning(const std::string& container_id)
{
    std::string container = this->inspectContainer(container_id);
    nlohmann::json container_json = nlohmann::json::parse(container);
    return container_json["State"]["Running"];
}