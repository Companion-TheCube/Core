/*
██████╗ ███████╗███╗   ███╗ ██████╗ ████████╗███████╗███████╗███████╗██████╗ ██╗   ██╗███████╗██████╗     ██████╗██████╗ ██████╗ 
██╔══██╗██╔════╝████╗ ████║██╔═══██╗╚══██╔══╝██╔════╝██╔════╝██╔════╝██╔══██╗██║   ██║██╔════╝██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
██████╔╝█████╗  ██╔████╔██║██║   ██║   ██║   █████╗  ███████╗█████╗  ██████╔╝██║   ██║█████╗  ██████╔╝   ██║     ██████╔╝██████╔╝
██╔══██╗██╔══╝  ██║╚██╔╝██║██║   ██║   ██║   ██╔══╝  ╚════██║██╔══╝  ██╔══██╗╚██╗ ██╔╝██╔══╝  ██╔══██╗   ██║     ██╔═══╝ ██╔═══╝ 
██║  ██║███████╗██║ ╚═╝ ██║╚██████╔╝   ██║   ███████╗███████║███████╗██║  ██║ ╚████╔╝ ███████╗██║  ██║██╗╚██████╗██║     ██║     
╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝ ╚═════╝    ╚═╝   ╚══════╝╚══════╝╚══════╝╚═╝  ╚═╝  ╚═══╝  ╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2025 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// TheCubeServerAPI implementation: manages HTTP client config and provides
// async chat-response retrieval. Many methods are TODO while the wire format
// and auth bootstrap are finalized.
#include "remoteServer.h"
#include <random>

// TheCubeServerAPI - class to interact with TheCube Server API. Will use API key stored in CubeDB. Key is stored encrypted and will be decrypted at load time.
// TODO: Implement this class

using namespace TheCubeServer;

TheCubeServerAPI::TheCubeServerAPI(std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioBuffer)
{
    // TODO:
    // Read the serial number from the hardwareInfo class
    this->serialNumber = "TESTING-SERIAL-NUMBER";
    // Read the API key from the CubeDB and decrypt it
    this->apiKey = "";
    // Read the auth key from the CubeDB and decrypt it
    this->authKey = "";

    this->cli = new httplib::Client(SERVER_API_URL);
    this->cli->set_default_headers({ { "Content-Type", "application/json" },
        { "ApiKey", this->apiKey } });
    this->cli->set_bearer_token_auth(this->authKey.c_str());
    this->cli->set_basic_auth(this->serialNumber.c_str(), serialNumberToPassword(this->serialNumber).c_str());
    if (!this->initServerConnection()) {
        CubeLog::error("Failed to initialize server connection");
        return;
    }

    // TODO: Read the setting for which AI service the user wants to use.
    // options include OpenAI, Google, Amazon, TheCubeServer
    // TheCubeServer is only available to premium subscribers. Includes Amazon, OpenAI, and Google.
    // Amazon is only available to plus subscribers. Includes OpenAI and Google.
    // OpenAi and Google are available to all subscribers.

    // register this->resetServerConnection() with the globalSettings class so that when the user changes their subscription level
    // this->resetServerConnection() will be called.

    // compare the services the user has access to with their preference
    // if they don't have access to the service they want, default to the next available service.

    CubeLog::info("TheCubeServerAPI initialized");
}

TheCubeServerAPI::~TheCubeServerAPI()
{
    CubeLog::info("TheCubeServerAPI closing");
    if (this->cli) {
        delete this->cli;
    }
    CubeLog::info("TheCubeServerAPI closed");
}

bool TheCubeServerAPI::initTranscribing()
{
    CubeLog::info("Initializing transcribing");
    // TODO:
    return true;
}

bool TheCubeServerAPI::streamAudio()
{
    CubeLog::info("Streaming audio");
    // TODO:
    return true;
}

bool TheCubeServerAPI::stopTranscribing()
{
    CubeLog::info("Stopping transcribing");
    // TODO:
    return true;
}

bool TheCubeServerAPI::initServerConnection()
{
    if (this->status == ServerStatus::SERVER_STATUS_READY) {
        CubeLog::info("Server connection already initialized");
        return true;
    }
    CubeLog::info("Initializing server connection");
    if (!ableToCommunicateWithRemoteServer()) {
        CubeLog::error("Unable to communicate with remote server");
        this->status = ServerStatus::SERVER_STATUS_ERROR;
        this->error = ServerError::SERVER_ERROR_CONNECTION_ERROR;
        return false;
    }
    if (this->apiKey.empty()) {
        CubeLog::error("API key is empty");
        this->status = ServerStatus::SERVER_STATUS_ERROR;
        this->error = ServerError::SERVER_ERROR_AUTHENTICATION_ERROR;
        return false;
    }
    if (this->authKey.empty()) {
        CubeLog::error("Auth key is empty");
        this->status = ServerStatus::SERVER_STATUS_ERROR;
        this->error = ServerError::SERVER_ERROR_AUTHENTICATION_ERROR;
        return false;
    }
    if (this->serialNumber.empty()) {
        CubeLog::error("Serial number is empty");
        this->status = ServerStatus::SERVER_STATUS_ERROR;
        this->error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
        return false;
    }
    auto res = this->cli->Get("/user/profile");
    if (!res) {
        CubeLog::error("Failed to get user profile");
        this->status = ServerStatus::SERVER_STATUS_ERROR;
        this->error = ServerError::SERVER_ERROR_CONNECTION_ERROR;
        return false;
    }
    switch (res->status) {
    case 200:
        CubeLog::info("User profile retrieved");
        break;
    case 400:
    case 401:
    case 403:
        CubeLog::error("Unauthorized");
        this->status = ServerStatus::SERVER_STATUS_ERROR;
        this->error = ServerError::SERVER_ERROR_AUTHENTICATION_ERROR;
        return false;
    case 418:
        CubeLog::error("Server indicates that it is a teapot. This is unusual since we are not making tea.");
        this->status = ServerStatus::SERVER_STATUS_ERROR;
        this->error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
        return false;
    case 500:
        CubeLog::error("Internal server error");
        this->status = ServerStatus::SERVER_STATUS_ERROR;
        this->error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
        return false;
    default:
        CubeLog::error("Unknown error");
        this->status = ServerStatus::SERVER_STATUS_ERROR;
        this->error = ServerError::SERVER_ERROR_UNKNOWN;
        return false;
    }
    try {
        nlohmann::json j = nlohmann::json::parse(res->body);
        if (j.contains("subscription_level")) {
            FourBit t2(j["subscription_level"]);
            this->services = t2;
        }
    } catch (std::exception& e) {
        CubeLog::error("Failed to parse user profile: " + std::string(e.what()));
        this->status = ServerStatus::SERVER_STATUS_ERROR;
        this->error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
        return false;
    }
    this->status = ServerStatus::SERVER_STATUS_READY;
    return true;
}

bool TheCubeServerAPI::resetServerConnection()
{
    CubeLog::info("Resetting server connection");
    this->error = ServerError::SERVER_ERROR_NONE;
    this->status = ServerStatus::SERVER_STATUS_INITIALIZING;
    this->state = ServerState::SERVER_STATE_IDLE;
    return this->initServerConnection();
}

bool TheCubeServerAPI::ableToCommunicateWithRemoteServer()
{
    CubeLog::info("Able to communicate with remote server");
    // TODO:
    // test connecting to the server to see if we can get a response + code 200. Response body should be "Hello."
    // - GET SERVER_API_URL/test

    return true;
}

TheCubeServerAPI::ServerStatus TheCubeServerAPI::getServerStatus()
{
    return status;
}

TheCubeServerAPI::ServerError TheCubeServerAPI::getServerError()
{
    return error;
}

TheCubeServerAPI::ServerState TheCubeServerAPI::getServerState()
{
    return state;
}

/**
 * @brief Get the Available Services as a bitfield.
 * @return TheCubeServerAPI::FourBit A bitfield of the available services.
 * bit 0 - OpenAI
 * bit 1 - Google
 * bit 2 - Amazon
 * bit 3 - TheCubeServer
 */
TheCubeServerAPI::FourBit TheCubeServerAPI::getAvailableServices()
{
    return services;
}

/**
 * @brief Get a chat response from the server.
 *
 * @param message The message to send to the LLM. Must contain all relevant information for the LLM to generate a response.
 * @param progressCB A callback function that will be called with the progress of the chat session. (optional)
 * @return std::future<std::string> A future that will contain the chat response when it is ready. Use .get() to retrieve the response and .wait_for() to check if the response is ready.
 */
// Post a message to the chat endpoint and poll for completion, invoking
// progressCB as the server streams interim messages. Returns a future that
// resolves to the final message string (or empty on error).
std::future<std::string> TheCubeServerAPI::getChatResponseAsync(const std::string& message, const std::function<void(std::string)>& progressCB)
{
    CubeLog::info("Getting chat response async");
    // generate a random number between 1 and 1000000
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000000);
    auto r = std::to_string(dis(gen));
    // This random number gets appended to the chat endpoint to create a unique chat session.
    // This allows for multiple chat sessions to be active at the same time for the same client.
    return std::async(std::launch::async, [&, r]() {
        auto res = this->cli->Post("/chat/" + r, message, "application/json");
        if (!res) {
            CubeLog::error("Failed to get chat response");
            return std::string();
        }
        switch (res->status) {
        case 200:
            if (res->body == "waiting") {
                CubeLog::info("Chat response initiated");
            }
            CubeLog::info("Chat response initiation failed");
            return res->body;
        case 400:
        case 401:
        case 403:
            CubeLog::error("Unauthorized");
            return std::string();
        case 418:
            CubeLog::error("Server indicates that it is a teapot. This is unusual since we are not making tea.");
            return std::string();
        case 500:
            CubeLog::error("Internal server error");
            return std::string();
        default:
            CubeLog::error("Unknown error");
            return std::string();
        }
        auto resCheck = [&, r]() {
            auto res = this->cli->Get("/chat/" + r);
            if (!res) {
                CubeLog::error("Failed to get chat response");
                return std::pair<bool, std::string>(false, "");
            }
            switch (res->status) {
            case 200: {
                CubeLog::info("Chat response retrieved");
                nlohmann::json j;
                try {
                    j = nlohmann::json::parse(res->body);
                } catch (std::exception& e) {
                    if (res->body == "waiting")
                        return std::pair<bool, std::string>(true, "");
                }
                if (j.contains("status") && j["status"] == "waiting" && j.contains("message")) {
                    progressCB(j["message"]);
                }
                if (j.contains("status") && j["status"] == "complete" && j.contains("message")) {
                    return std::pair<bool, std::string>(true, j["message"]);
                }
                return std::pair<bool, std::string>(true, "");
            }
            case 400:
            case 401:
            case 403:
                CubeLog::error("Unauthorized");
                return std::pair<bool, std::string>(false, "");
            case 418:
                CubeLog::error("Server indicates that it is a teapot. This is unusual since we are not making tea.");
                return std::pair<bool, std::string>(false, "");
            case 500:
                CubeLog::error("Internal server error");
                return std::pair<bool, std::string>(false, "");
            default:
                CubeLog::error("Unknown error");
                return std::pair<bool, std::string>(false, "");
            }
        };
        while (resCheck().first == true && resCheck().second.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (resCheck().first == false) {
            CubeLog::error("Failed to get chat response");
            return std::string();
        }
        return resCheck().second;
    });
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Derive a password-like token from a device serial number by base64-encoding,
// hashing (SHA-256), and CRC32 post-processing to a shorter string.
std::string TheCubeServer::serialNumberToPassword(const std::string& serialNumber)
{
    std::string output;
    // base64 encode the serial number
    output = base64_encode_cube(serialNumber);
    // hash the base64 encoded serial number
    output = sha256(output);
    // CRC32 the hashed serial number to get something shorter and return it
    return crc32(output);
}
