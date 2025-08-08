#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "../../src/api/api.h"
#include "../../src/api/authentication.h"
#include "../../src/database/cubeDB.h"
#include "../../src/utils.h"

// Integration test: start API, add CubeAuth endpoints and a temporary PRIVATE
// endpoint, mint a token via the auth flow, and call the private endpoint with
// Authorization header. Verifies that JSON body from handler is preserved.

static void addInterfaceEndpoints(API& api, I_API_Interface& iface)
{
    auto name = iface.getInterfaceName();
    auto endpoints = iface.getHttpEndpointData();
    for (auto& e : endpoints) {
        auto type = std::get<0>(e);
        auto action = std::get<1>(e);
        auto epName = std::get<2>(e);
        api.addEndpoint(name + "-" + epName, "/" + name + "-" + epName, type, action);
    }
}

TEST(ApiIntegration, PrivateEndpointHonorsResponseBody)
{
    // Use a temporary working directory so DB files and sockets are isolated per test run
    namespace fs = std::filesystem;
    auto prevCwd = fs::current_path();
    auto randSuffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    auto tmpRoot = fs::temp_directory_path() / (std::string("cube_api_test_") + randSuffix);
    fs::create_directories(tmpRoot);
    fs::current_path(tmpRoot);

    // Initialize DBs for auth module
    auto dbm = std::make_shared<CubeDatabaseManager>();
    CubeDB::setCubeDBManager(dbm);
    // Blobs manager required by CubeDB interface (not used directly here)
    auto blobs = std::make_shared<BlobsManager>(dbm, "data/blobs.db");
    CubeDB::setBlobsManager(blobs);
    dbm->openAll();

    // Build API and register CubeAuth endpoints + a temporary PRIVATE endpoint
    API api;
    // Bind to test port from .env (HTTP_PORT_TEST) to keep configurable
    int httpPort = 55281;
    try {
        auto s = Config::get("HTTP_PORT_TEST", "55281");
        httpPort = std::stoi(s);
    } catch (...) {
        httpPort = 55281;
    }
    api.setHttpBinding("127.0.0.1", httpPort);
    // Use centralized Config to get test-specific IPC socket path
    std::string ipcPath = Config::get("IPC_SOCKET_PATH_TEST", "");
    if (ipcPath.empty()) ipcPath = "test_ipc.sock";
    api.setIpcPath(ipcPath);
    CubeAuth auth; // sets keys and exposes endpoints
    addInterfaceEndpoints(api, auth);

    // Temporary private endpoint that echoes JSON and returns success with empty errorString
    const std::string testPath = "/Test-privateEcho";
    api.addEndpoint("Test-privateEcho", testPath, PRIVATE_ENDPOINT | POST_ENDPOINT,
        [](const httplib::Request& req, httplib::Response& res) {
            nlohmann::json j; j["ok"] = true; j["payload"] = nlohmann::json::parse(req.body);
            res.set_content(j.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        });

    // Start API server (HTTP + IPC); HTTP listens on 0.0.0.0:55280
    api.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 1) Get initial code (and ask it to be returned) to create client row
    httplib::Client http((std::string("http://127.0.0.1:") + std::to_string(httpPort)).c_str());
    const std::string clientID = "test-client-123";
    auto resInit = http.Get(("/CubeAuth-initCode?client_id=" + clientID + "&return_code=true").c_str());
    ASSERT_TRUE(resInit);
    ASSERT_EQ(resInit->status, 200);
    auto jInit = nlohmann::json::parse(resInit->body);
    ASSERT_TRUE(jInit.contains("initial_code"));
    const std::string initialCode = jInit["initial_code"].get<std::string>();

    // 2) Exchange initial code for an auth token
    auto resAuth = http.Get(("/CubeAuth-authHeader?client_id=" + clientID + "&initial_code=" + initialCode).c_str());
    ASSERT_TRUE(resAuth);
    ASSERT_EQ(resAuth->status, 200);
    auto jAuth = nlohmann::json::parse(resAuth->body);
    ASSERT_TRUE(jAuth.contains("auth_code"));
    const std::string token = jAuth["auth_code"].get<std::string>();

    // 3) Call private endpoint with Authorization header; ensure JSON body is preserved
    httplib::Headers hdrs = { { "Authorization", std::string("Bearer ") + token }, { "Content-Type", "application/json" } };
    nlohmann::json payload; payload["msg"] = "hello";
    auto resTest = http.Post(testPath.c_str(), hdrs, payload.dump(), "application/json");
    ASSERT_TRUE(resTest);
    ASSERT_EQ(resTest->status, 200);
    ASSERT_EQ(resTest->get_header_value("Content-Type"), std::string("application/json"));
    auto jResp = nlohmann::json::parse(resTest->body);
    EXPECT_TRUE(jResp["ok"].get<bool>());
    EXPECT_EQ(jResp["payload"]["msg"].get<std::string>(), "hello");

    api.stop();

    // Restore previous working directory and clean up temp files
    fs::current_path(prevCwd);
    std::error_code ec; fs::remove_all(tmpRoot, ec);
}
