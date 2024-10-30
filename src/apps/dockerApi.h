#pragma once
#ifndef DOCKERAPI_H
#define DOCKERAPI_H
#ifndef HTTPLIB_INCLUDED
#define HTTPLIB_INCLUDED
#include <httplib.h>
#endif
#ifndef WIN32_INCLUDED
#define WIN32_INCLUDED
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#endif
#include <expected>
#include <iostream>
#include <logger.h>
#include <nlohmann/json.hpp>
#include <string>
#include <utils.h>
#include <vector>

class DockerError {
    static unsigned long errorCounter;

public:
    enum class ErrorCodes: unsigned int {
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
        JSON_PARSE_ERROR = 27
    };
    const std::map<ErrorCodes, std::string> ErrorCodeMap = {
        { ErrorCodes::CONTAINER_NOT_FOUND, "CONTAINER_NOT_FOUND" },
        { ErrorCodes::CONTAINER_ALREADY_RUNNING, "CONTAINER_ALREADY_RUNNING" },
        { ErrorCodes::CONTAINER_NOT_RUNNING, "CONTAINER_NOT_RUNNING" },
        { ErrorCodes::CONTAINER_KILL_ERROR, "CONTAINER_KILL_ERROR" },
        { ErrorCodes::CONTAINER_STOP_ERROR, "CONTAINER_STOP_ERROR" },
        { ErrorCodes::CONTAINER_START_ERROR, "CONTAINER_START_ERROR" },
        { ErrorCodes::CONTAINER_INSPECT_ERROR, "CONTAINER_INSPECT_ERROR" },
        { ErrorCodes::CONTAINER_IS_RUNNING_ERROR, "CONTAINER_IS_RUNNING_ERROR" },
        { ErrorCodes::CONTAINER_NOT_FOUND_ERROR, "CONTAINER_NOT_FOUND_ERROR" },
        { ErrorCodes::CONTAINER_LIST_ERROR, "CONTAINER_LIST_ERROR" },
        { ErrorCodes::IMAGE_LIST_ERROR, "IMAGE_LIST_ERROR" },
        { ErrorCodes::IMAGE_NOT_FOUND, "IMAGE_NOT_FOUND" },
        { ErrorCodes::IMAGE_PULL_ERROR, "IMAGE_PULL_ERROR" },
        { ErrorCodes::IMAGE_REMOVE_ERROR, "IMAGE_REMOVE_ERROR" },
        { ErrorCodes::IMAGE_INSPECT_ERROR, "IMAGE_INSPECT_ERROR" },
        { ErrorCodes::IMAGE_HISTORY_ERROR, "IMAGE_HISTORY_ERROR" },
        { ErrorCodes::IMAGE_BUILD_ERROR, "IMAGE_BUILD_ERROR" },
        { ErrorCodes::IMAGE_TAG_ERROR, "IMAGE_TAG_ERROR" },
        { ErrorCodes::IMAGE_PUSH_ERROR, "IMAGE_PUSH_ERROR" },
        { ErrorCodes::IMAGE_SAVE_ERROR, "IMAGE_SAVE_ERROR" },
        { ErrorCodes::IMAGE_LOAD_ERROR, "IMAGE_LOAD_ERROR" },
        { ErrorCodes::IMAGE_EXPORT_ERROR, "IMAGE_EXPORT_ERROR" },
        { ErrorCodes::IMAGE_IMPORT_ERROR, "IMAGE_IMPORT_ERROR" },
        { ErrorCodes::IMAGE_PRUNE_ERROR, "IMAGE_PRUNE_ERROR" },
        { ErrorCodes::NETWORK_LIST_ERROR, "NETWORK_LIST_ERROR" },
        { ErrorCodes::NETWORK_CREATE_ERROR, "NETWORK_CREATE_ERROR" },
        { ErrorCodes::JSON_PARSE_ERROR, "JSON_PARSE_ERROR" }
    };
    std::string message;
    ErrorCodes code;
    DockerError(const std::string& message, ErrorCodes code);
    unsigned long getErrorCounter();
};

class DockerAPI {
private:
    std::unique_ptr<httplib::Client> client;
    std::string base_url;
    void printDockerInfo();

public:
    DockerAPI(const std::string& base_url);
    DockerAPI();
    ~DockerAPI();
    std::expected<std::string, DockerError> getContainers_json();
    std::expected<std::string, DockerError> getImages_json();
    std::expected<std::string, DockerError> startContainer(const std::string& container_id);
    std::expected<std::string, DockerError> stopContainer(const std::string& container_id);
    std::expected<std::string, DockerError> killContainer(const std::string& container_id);
    std::expected<std::string, DockerError> inspectContainer_json(const std::string& container_id);
    std::expected<bool, DockerError> isContainerRunning(const std::string& container_id);
    std::expected<std::vector<std::string>, DockerError> getContainers_vec();
    std::expected<std::vector<std::string>, DockerError> getImages_vec();
};

#endif