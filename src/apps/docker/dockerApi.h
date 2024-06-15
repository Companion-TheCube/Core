#include <httplib.h>
#include <logger.h>
#include <utils.h>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include <vector>
#include <expected>

class DockerError {
public:
    std::string message;
    int code;
    DockerError(std::string message, int code);
    enum ErrorCodes {
        CONTAINER_NOT_FOUND = 1,
        CONTAINER_ALREADY_RUNNING = 2,
        CONTAINER_NOT_RUNNING = 3,
        CONTAINER_KILL_ERROR = 4,
        CONTAINER_STOP_ERROR = 5,
        CONTAINER_START_ERROR = 6,
        CONTAINER_INSPECT_ERROR = 7,
        CONTAINER_IS_RUNNING_ERROR = 8,
        CONTAINER_NOT_FOUND_ERROR = 9,
        CONTAINER_LIST_ERROR = 10,
        IMAGE_LIST_ERROR = 11,
        IMAGE_NOT_FOUND = 12,
        IMAGE_PULL_ERROR = 13,
        IMAGE_REMOVE_ERROR = 14,
        IMAGE_INSPECT_ERROR = 15,
        IMAGE_HISTORY_ERROR = 16,
        IMAGE_BUILD_ERROR = 17,
        IMAGE_TAG_ERROR = 18,
        IMAGE_PUSH_ERROR = 19,
        IMAGE_SAVE_ERROR = 20,
        IMAGE_LOAD_ERROR = 21,
        IMAGE_EXPORT_ERROR = 22,
        IMAGE_IMPORT_ERROR = 23,
        IMAGE_PRUNE_ERROR = 24,
        NETWORK_LIST_ERROR = 25,
        NETWORK_CREATE_ERROR = 26,
    };

    const std::map<ErrorCodes, std::string> ErrorCodeMap = {
        {CONTAINER_NOT_FOUND, "CONTAINER_NOT_FOUND"},
        {CONTAINER_ALREADY_RUNNING, "CONTAINER_ALREADY_RUNNING"},
        {CONTAINER_NOT_RUNNING, "CONTAINER_NOT_RUNNING"},
        {CONTAINER_KILL_ERROR, "CONTAINER_KILL_ERROR"},
        {CONTAINER_STOP_ERROR, "CONTAINER_STOP_ERROR"},
        {CONTAINER_START_ERROR, "CONTAINER_START_ERROR"},
        {CONTAINER_INSPECT_ERROR, "CONTAINER_INSPECT_ERROR"},
        {CONTAINER_IS_RUNNING_ERROR, "CONTAINER_IS_RUNNING_ERROR"},
        {CONTAINER_NOT_FOUND_ERROR, "CONTAINER_NOT_FOUND_ERROR"},
        {CONTAINER_LIST_ERROR, "CONTAINER_LIST_ERROR"},
        {IMAGE_LIST_ERROR, "IMAGE_LIST_ERROR"},
        {IMAGE_NOT_FOUND, "IMAGE_NOT_FOUND"},
        {IMAGE_PULL_ERROR, "IMAGE_PULL_ERROR"},
        {IMAGE_REMOVE_ERROR, "IMAGE_REMOVE_ERROR"},
        {IMAGE_INSPECT_ERROR, "IMAGE_INSPECT_ERROR"},
        {IMAGE_HISTORY_ERROR, "IMAGE_HISTORY_ERROR"},
        {IMAGE_BUILD_ERROR, "IMAGE_BUILD_ERROR"},
        {IMAGE_TAG_ERROR, "IMAGE_TAG_ERROR"},
        {IMAGE_PUSH_ERROR, "IMAGE_PUSH_ERROR"},
        {IMAGE_SAVE_ERROR, "IMAGE_SAVE_ERROR"},
        {IMAGE_LOAD_ERROR, "IMAGE_LOAD_ERROR"},
        {IMAGE_EXPORT_ERROR, "IMAGE_EXPORT_ERROR"},
        {IMAGE_IMPORT_ERROR, "IMAGE_IMPORT_ERROR"},
        {IMAGE_PRUNE_ERROR, "IMAGE_PRUNE_ERROR"},
        {NETWORK_LIST_ERROR, "NETWORK_LIST_ERROR"},
        {NETWORK_CREATE_ERROR, "NETWORK_CREATE_ERROR"}
    };
};

class DockerAPI {
private:
    httplib::Client client;
    std::string base_url;
public:
    DockerAPI(const std::string& base_url);
    DockerAPI();
    std::string getContainers_json();
    std::string getImages_json();
    std::expected<std::string, DockerError> startContainer(const std::string& container_id);
    std::expected<std::string, DockerError> stopContainer(const std::string& container_id);
    std::expected<std::string, DockerError> killContainer(const std::string& container_id);
    std::string inspectContainer(const std::string& container_id);
    bool isContainerRunning(const std::string& container_id);
    std::vector<std::string> getContainers_vec();
};
