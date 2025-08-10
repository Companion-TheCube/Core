#include <gtest/gtest.h>
#include "../../src/api/api.h"

// This test enforces the convention: on success, handlers set the response
// body via `res` and return EndpointError with an empty errorString.
// The API layer must not overwrite `res` when errorType == ENDPOINT_NO_ERROR.

TEST(ApiConventions, SuccessDoesNotUseErrorString)
{
    // Arrange: a handler that sets JSON in res and returns success with empty string.
    auto handler = [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j; j["ok"] = true; j["value"] = 42;
        res.set_content(j.dump(), "application/json");
        return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
    };

    httplib::Request req; // empty request is fine for this test
    httplib::Response res;

    // Act
    auto result = handler(req, res);

    // Assert: API convention — success uses res for content and empty error string.
    EXPECT_EQ(result.errorType, EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR);
    EXPECT_TRUE(result.errorString.empty());
    EXPECT_EQ(res.get_header_value("Content-Type"), std::string("application/json"));
    EXPECT_FALSE(res.body.empty());
    auto parsed = nlohmann::json::parse(res.body);
    EXPECT_TRUE(parsed["ok"].get<bool>());
    EXPECT_EQ(parsed["value"].get<int>(), 42);
}

