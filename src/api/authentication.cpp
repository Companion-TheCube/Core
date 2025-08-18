/*
 █████╗ ██╗   ██╗████████╗██╗  ██╗███████╗███╗   ██╗████████╗██╗ ██████╗ █████╗ ████████╗██╗ ██████╗ ███╗   ██╗    ██████╗██████╗ ██████╗ 
██╔══██╗██║   ██║╚══██╔══╝██║  ██║██╔════╝████╗  ██║╚══██╔══╝██║██╔════╝██╔══██╗╚══██╔══╝██║██╔═══██╗████╗  ██║   ██╔════╝██╔══██╗██╔══██╗
███████║██║   ██║   ██║   ███████║█████╗  ██╔██╗ ██║   ██║   ██║██║     ███████║   ██║   ██║██║   ██║██╔██╗ ██║   ██║     ██████╔╝██████╔╝
██╔══██║██║   ██║   ██║   ██╔══██║██╔══╝  ██║╚██╗██║   ██║   ██║██║     ██╔══██║   ██║   ██║██║   ██║██║╚██╗██║   ██║     ██╔═══╝ ██╔═══╝ 
██║  ██║╚██████╔╝   ██║   ██║  ██║███████╗██║ ╚████║   ██║   ██║╚██████╗██║  ██║   ██║   ██║╚██████╔╝██║ ╚████║██╗╚██████╗██║     ██║     
╚═╝  ╚═╝ ╚═════╝    ╚═╝   ╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═╝ ╚═════╝╚═╝  ╚═╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝ ╚═════╝╚═╝     ╚═╝
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

#ifndef LOGGER_H
#include <logger.h>
#endif
#include "authentication.h"
#include <chrono>
#include <thread>

/**
 * @brief Generate a random key of a given length using the charset of 0-9, A-Z, a-z
 *
 * @param length
 * @return std::string
 */
std::string KeyGenerator(size_t length)
{
    CubeLog::debug("Generating key of length " + std::to_string(length));
    std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string result;
    result.resize(length);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, charset.length() - 1);
    for (size_t i = 0; i < length; i++) {
        result[i] = charset[dis(gen)];
    }
    CubeLog::debug("Generated key: " + result);
    return result;
}

/**
 * @brief Generate a random 6 character code using the charset of 0-9 and A-Z
 *
 * @return std::string
 */
std::string Code6Generator()
{
    CubeLog::debug("Generating 6 character code");
    std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string result;
    result.resize(6);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, charset.length() - 1);
    for (size_t i = 0; i < 6; i++) {
        result[i] = charset[dis(gen)];
    }
    CubeLog::debug("Generated 6 character code: " + result);
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////

bool CubeAuth::available = false;
std::string CubeAuth::privateKey = "";
std::string CubeAuth::publicKey = "";
bool CubeAuth::cubeAuthStaticKeysSet = false;
std::string CubeAuth::lastError = "";

// Create the client_tokens table if it does not exist
bool CubeAuth::ensureTokenTable()
{
    Database* db = CubeDB::getDBManager()->getDatabase("auth");
    if (!db->isOpen()) return false;
    if (!db->tableExists("client_tokens")) {
        return db->createTable("client_tokens",
            { "id", "client_id", "token", "issued_at_ms", "expires_at_ms", "last_used_ms", "revoked", "revoked_at_ms" },
            { "INTEGER PRIMARY KEY", "TEXT", "TEXT", "INTEGER", "INTEGER", "INTEGER", "INTEGER", "INTEGER" },
            { true, false, true, false, false, false, false, false });
    }
    return true;
}

std::string CubeAuth::stripBearer(const std::string& header)
{
    const std::string prefix = "Bearer ";
    if (header.rfind(prefix, 0) == 0) return header.substr(prefix.size());
    return header;
}

/**
 * @brief Construct a new CubeAuth object
 *
 */
CubeAuth::CubeAuth()
{
    CubeLog::info("Authentication module starting...");
    if (sodium_init() == -1) {
        CubeLog::error("Failed to initialize libsodium.");
        CubeAuth::lastError = "Failed to initialize libsodium.";
        return;
    }
    if (!CubeAuth::cubeAuthStaticKeysSet) {
        std::pair<std::string, std::string> keyPair = generateKeyPair();
        CubeAuth::privateKey = keyPair.second;
        CubeAuth::publicKey = keyPair.first;
        // load the public and private keys from .pem files
        std::ifstream private_key_file("private_key.pem");
        std::ifstream public_key_file("public_key.pem");
        if (private_key_file.is_open() && public_key_file.is_open()) {
            std::string private_key((std::istreambuf_iterator<char>(private_key_file)), std::istreambuf_iterator<char>());
            std::string public_key((std::istreambuf_iterator<char>(public_key_file)), std::istreambuf_iterator<char>());
            CubeAuth::privateKey = private_key;
            CubeAuth::publicKey = public_key;
            CubeLog::info("Loaded public and private keys from .pem files.");
        } else {
            CubeLog::info("Generated public and private keys.");
        }
    } else {
        CubeLog::debug("Using previously set public and private keys for CubeAuth.");
    }
    CubeAuth::available = true;
}

/**
 * @brief Destroy the CubeAuth object
 *
 */
CubeAuth::~CubeAuth()
{
    CubeLog::info("Authentication module stopping...");
}

// TODO: add checkAuth function that only takes the app_id and returns bool if the app_id has been allowed by the user

/**
 * @brief Check the authentication code
 *
 * @param privateKey
 * @param app_id
 * @param encrypted_auth_code
 * @return int CubeAuth::AUTH_CODE
 */
CubeAuth::AUTH_CODE CubeAuth::checkAuth(const std::string &privateKey, const std::string &app_id, const std::string &encrypted_auth_code)
{
    if (!CubeAuth::available) {
        CubeLog::error("Authentication module not available.");
        CubeAuth::lastError = "Authentication module not available.";
        return AUTH_CODE::AUTH_FAIL_UNKNOWN;
    }
    if (privateKey.length() != 64) {
        CubeLog::error("Invalid private key length.");
        CubeAuth::lastError = "Invalid private key length.";
        return AUTH_CODE::AUTH_FAIL_INVALID_PRIVATE_KEY;
    }
    if (app_id.length() != CUBE_APPS_ID_LENGTH) {
        CubeLog::error("Invalid app id length.");
        CubeAuth::lastError = "Invalid app id length.";
        return AUTH_CODE::AUTH_FAIL_INVALID_APP_ID_LEN;
    }
    Database* db = CubeDB::getDBManager()->getDatabase("auth");
    if (!db->isOpen()) {
        CubeLog::error("Database not open.");
        CubeAuth::lastError = "Database not open.";
        return AUTH_CODE::AUTH_FAIL_DB_NOT_OPEN;
    }
    // check to see if the app_id exists
    if (!db->rowExists(DB_NS::TableNames::APPS, "app_id = '" + app_id + "'")) {
        CubeLog::error("App id not found.");
        CubeAuth::lastError = "App id not found.";
        return AUTH_CODE::AUTH_FAIL_INVALID_APP_ID;
    }
    // get the public key
    std::vector<std::vector<std::string>> pub_key = db->selectData(DB_NS::TableNames::APPS, { "public_key" }, "app_id = '" + app_id + "'");
    // get the auth_code
    std::vector<std::vector<std::string>> auth_code = db->selectData(DB_NS::TableNames::APPS, { "auth_code" }, "app_id = '" + app_id + "'");
    // decrypt the encrypted_auth_code
    unsigned char decrypted_auth_code[crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES + 6];
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    unsigned char key[crypto_secretbox_KEYBYTES];
    unsigned char* encrypted_auth_code_ptr = (unsigned char*)encrypted_auth_code.c_str();
    unsigned char* public_key_ptr = (unsigned char*)pub_key[0][0].c_str();
    unsigned char* private_key_ptr = (unsigned char*)privateKey.c_str();
    if (crypto_box_beforenm(key, public_key_ptr, private_key_ptr) != 0) {
        CubeLog::error("Failed to generate key.");
        CubeAuth::lastError = "Failed to generate key.";
        return AUTH_CODE::AUTH_FAIL_KEY_GEN;
    }
    memcpy(nonce, encrypted_auth_code_ptr, crypto_secretbox_NONCEBYTES);
    memcpy(decrypted_auth_code, encrypted_auth_code_ptr + crypto_secretbox_NONCEBYTES, crypto_secretbox_MACBYTES + 6);
    if (crypto_secretbox_open_easy(decrypted_auth_code + crypto_secretbox_MACBYTES, decrypted_auth_code, crypto_secretbox_MACBYTES + 6, nonce, key) != 0) {
        CubeLog::error("Failed to decrypt auth code.");
        CubeAuth::lastError = "Failed to decrypt auth code.";
        return AUTH_CODE::AUTH_FAIL_DECRYPT;
    }
    std::string decrypted_auth_code_str((char*)decrypted_auth_code + crypto_secretbox_MACBYTES);
    // compare the decrypted auth code with the auth code in the database
    if (decrypted_auth_code_str != auth_code[0][0]) {
        CubeLog::error("Auth code mismatch.");
        CubeAuth::lastError = "Auth code mismatch.";
        return AUTH_CODE::AUTH_FAIL_CODE_MISMATCH;
    }
    return AUTH_CODE::AUTH_SUCCESS;
}

/**
 * @brief Generate a random 6 character code
 *
 * @return std::string
 */
std::string CubeAuth::generateAuthCode()
{
    return Code6Generator();
}

/**
 * @brief Generate a public/private key pair
 *
 * @return std::pair<std::string, std::string> First is the public key, second is the private key
 */
std::pair<std::string, std::string> CubeAuth::generateKeyPair()
{
    CubeLog::debug("Generating key pair.");
    std::string public_key(crypto_box_PUBLICKEYBYTES, 0);
    std::string private_key(crypto_box_SECRETKEYBYTES, 0);
    crypto_box_keypair((unsigned char*)public_key.c_str(), (unsigned char*)private_key.c_str());
    return std::make_pair(public_key, private_key);
}

/**
 * @brief Encrypt an auth code using a public key
 *
 * @param auth_code
 * @param public_key
 * @return std::string
 */
std::string CubeAuth::encryptAuthCode(const std::string& auth_code, const std::string& public_key)
{
    CubeLog::debug("Encrypting auth code.");
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    unsigned char key[crypto_secretbox_KEYBYTES];
    unsigned char encrypted_auth_code[crypto_secretbox_MACBYTES + 6];
    unsigned char* auth_code_ptr = (unsigned char*)auth_code.c_str();
    unsigned char* public_key_ptr = (unsigned char*)public_key.c_str();
    unsigned char* private_key_ptr = (unsigned char*)CubeAuth::privateKey.c_str();
    if (crypto_box_beforenm(key, public_key_ptr, private_key_ptr) != 0) {
        CubeLog::error("Failed to generate key.");
        CubeAuth::lastError = "Failed to generate key.";
        return "";
    }
    randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
    if (crypto_secretbox_easy(encrypted_auth_code, auth_code_ptr, 6, nonce, key) != 0) {
        CubeLog::error("Failed to encrypt auth code.");
        CubeAuth::lastError = "Failed to encrypt auth code.";
        return "";
    }
    std::string encrypted_auth_code_str((char*)nonce, crypto_secretbox_NONCEBYTES);
    encrypted_auth_code_str.append((char*)encrypted_auth_code, crypto_secretbox_MACBYTES + 6);
    return encrypted_auth_code_str;
}

/**
 * @brief Decrypt an auth code using a private key
 *
 * @param auth_code
 * @param private_key
 * @return std::string
 */
std::string CubeAuth::decryptAuthCode(const std::string& auth_code, const std::string& private_key)
{
    CubeLog::debug("Decrypting auth code.");
    unsigned char decrypted_auth_code[6];
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    unsigned char key[crypto_secretbox_KEYBYTES];
    unsigned char* encrypted_auth_code_ptr = (unsigned char*)auth_code.c_str();
    unsigned char* private_key_ptr = (unsigned char*)private_key.c_str();
    unsigned char* public_key_ptr = (unsigned char*)CubeAuth::publicKey.c_str();
    if (crypto_box_beforenm(key, public_key_ptr, private_key_ptr) != 0) {
        CubeLog::error("Failed to generate key.");
        CubeAuth::lastError = "Failed to generate key.";
        return "";
    }
    memcpy(nonce, encrypted_auth_code_ptr, crypto_secretbox_NONCEBYTES);
    if (crypto_secretbox_open_easy(decrypted_auth_code, encrypted_auth_code_ptr + crypto_secretbox_NONCEBYTES, crypto_secretbox_MACBYTES + 6, nonce, key) != 0) {
        CubeLog::error("Failed to decrypt auth code.");
        CubeAuth::lastError = "Failed to decrypt auth code.";
        return "";
    }
    return std::string((char*)decrypted_auth_code, 6);
}

/**
 * @brief Encrypt data using a public key
 *
 * @param data
 * @param public_key
 * @return std::string
 */
std::string CubeAuth::encryptData(const std::string& data, const std::string& public_key)
{
    CubeLog::debug("Encrypting data. Length: " + std::to_string(data.length()) + " bytes.");
    if (data.length() > 65535) {
        CubeLog::error("Data too large.");
        CubeAuth::lastError = "Data too large.";
        return "";
    }
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    unsigned char key[crypto_secretbox_KEYBYTES];

    unsigned char* data_ptr = (unsigned char*)data.c_str();
    unsigned char* public_key_ptr = (unsigned char*)public_key.c_str();
    unsigned char* private_key_ptr = (unsigned char*)CubeAuth::privateKey.c_str();
    if (crypto_box_beforenm(key, public_key_ptr, private_key_ptr) != 0) {
        CubeLog::error("Failed to generate key.");
        CubeAuth::lastError = "Failed to generate key.";
        return "";
    }
    unsigned char* encrypted_data = new unsigned char[data.length() + crypto_secretbox_MACBYTES];
    randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
    if (crypto_secretbox_easy(encrypted_data, data_ptr, data.length(), nonce, key) != 0) {
        CubeLog::error("Failed to encrypt data.");
        CubeAuth::lastError = "Failed to encrypt data.";
        delete[] encrypted_data;
        return "";
    }
    std::string encrypted_data_str((char*)nonce, crypto_secretbox_NONCEBYTES);
    encrypted_data_str.append((char*)encrypted_data, data.length() + crypto_secretbox_MACBYTES);
    delete[] encrypted_data;
    return encrypted_data_str;
}

/**
 * @brief Decrypt data using a private key
 *
 * @param data
 * @param private_key
 * @param length
 * @return std::string
 */
std::string CubeAuth::decryptData(const std::string& data, const std::string& private_key, size_t length)
{
    CubeLog::debug("Decrypting data. Length: " + std::to_string(length) + " bytes.");

    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    unsigned char key[crypto_secretbox_KEYBYTES];
    unsigned char* encrypted_data_ptr = (unsigned char*)data.c_str();
    unsigned char* private_key_ptr = (unsigned char*)private_key.c_str();
    unsigned char* public_key_ptr = (unsigned char*)CubeAuth::publicKey.c_str();
    if (crypto_box_beforenm(key, public_key_ptr, private_key_ptr) != 0) {
        CubeLog::error("Failed to generate key.");
        CubeAuth::lastError = "Failed to generate key.";
        return "";
    }
    unsigned char* decrypted_data = new unsigned char[length];
    memcpy(nonce, encrypted_data_ptr, crypto_secretbox_NONCEBYTES);
    if (crypto_secretbox_open_easy(decrypted_data, encrypted_data_ptr + crypto_secretbox_NONCEBYTES, crypto_box_MACBYTES + length, nonce, key) != 0) {
        CubeLog::error("Failed to decrypt data.");
        CubeAuth::lastError = "Failed to decrypt data.";
        delete[] decrypted_data;
        return "";
    }
    std::string decrypted_data_str((char*)decrypted_data, length);
    delete[] decrypted_data;
    return decrypted_data_str;
}

/**
 * @brief Get the last error message
 *
 * @return std::string
 */
std::string CubeAuth::getLastError()
{
    CubeLog::debug("Getting last error: " + CubeAuth::lastError);
    return CubeAuth::lastError;
}

/**
 * @brief Check if the client is authorized using the Authorization header
 *
 * @param authHeader
 * @return bool
 */
bool CubeAuth::isAuthorized_authHeader(const std::string& authHeader)
{
    // check the db for the auth code
    auto token = stripBearer(authHeader);
    Database* db = CubeDB::getDBManager()->getDatabase("auth");
    if (!db->isOpen()) {
        CubeLog::error("Database not open.");
        CubeAuth::lastError = "Database not open.";
        return false;
    }
    // fast path: token must match an active client's stored token
    auto auth_code = db->selectData(DB_NS::TableNames::CLIENTS, { "client_id", "auth_code" }, "auth_code = '" + token + "'");
    if (auth_code.empty()) {
        CubeLog::error("Auth code not found.");
        CubeAuth::lastError = "Auth code not found.";
        return false;
    }
    // extended checks: expiry and revocation
    if (!ensureTokenTable()) {
        CubeLog::warning("Token table missing; skipping expiry/revocation checks");
        return true;
    }
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    auto tokenRows = db->selectData("client_tokens", { "revoked", "expires_at_ms" }, "token = '" + token + "'");
    if (tokenRows.empty()) {
        CubeLog::error("Token metadata not found");
        return false;
    }
    bool revoked = tokenRows[0][0] == "1";
    long long expiresAt = 0;
    try { expiresAt = std::stoll(tokenRows[0][1]); } catch (...) { expiresAt = 0; }
    if (revoked) {
        CubeLog::error("Token revoked");
        return false;
    }
    if (expiresAt > 0 && nowMs > expiresAt) {
        CubeLog::error("Token expired");
        return false;
    }
    // update last_used
    db->updateData("client_tokens", { "last_used_ms" }, { std::to_string(nowMs) }, "token = '" + token + "'");
    return true;
}

/**
 * @brief Get the name of the interface
 *
 * @return std::string
 */
constexpr std::string CubeAuth::getInterfaceName() const
{
    return "CubeAuth";
}

/**
 * @brief Get the HTTP endpoint data
 *
 * @return HttpEndPointData_t
 */
HttpEndPointData_t CubeAuth::getHttpEndpointData()
{
    HttpEndPointData_t data;
    data.push_back({ PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            // get the appID from the query string
            const std::string clientID = req.get_param_value("client_id");
            const std::string initialCode = req.get_param_value("initial_code");
            CubeLog::debug("GET authHeader called with client_id: " + clientID);
            // first we generate a random string of random length between 30 and 50 characters
            std::string randomString = KeyGenerator(30 + (rand() % 21));
            CubeLog::debug("Generated random string: " + randomString);
            // then we encrypt the random string using the public key
            std::string encryptedString = CubeAuth::encryptData(randomString, CubeAuth::publicKey);
            CubeLog::debug("Encrypted random string: " + encryptedString);
            // then we store this in the db
            Database* db = CubeDB::getDBManager()->getDatabase("auth");
            if (!db->isOpen()) {
                CubeLog::error("Database not open.");
                nlohmann::json j;
                j["success"] = false;
                j["message"] = "Database not open.";
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Database not open.");
            }
            // check to see if the client_id exists
            if (!db->rowExists(DB_NS::TableNames::CLIENTS, "client_id = '" + clientID + "'")) {
                CubeLog::error("Client id not found.");
                nlohmann::json j;
                j["success"] = false;
                j["message"] = "Client id not found.";
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Client id not found.");
            }
            // check to see if the initial code matches
            if (!db->rowExists(DB_NS::TableNames::CLIENTS, "client_id = '" + clientID + "' AND initial_code = '" + initialCode + "'")) {
                CubeLog::error("Initial code mismatch.");
                nlohmann::json j;
                j["success"] = false;
                j["message"] = "Initial code mismatch.";
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Initial code mismatch.");
            }
            // Build a header-safe token and store it for the client
            std::string token = base64_encode_cube(encryptedString);
            if (!db->updateData(DB_NS::TableNames::CLIENTS, { "auth_code" }, { token }, "client_id = '" + clientID + "'")) {
                CubeLog::error("Failed to set auth code.");
                nlohmann::json j;
                j["success"] = false;
                j["message"] = "Failed to set auth code.";
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Failed to set auth code.");
            }
            // Upsert token metadata with expiry and timestamps
            ensureTokenTable();
            auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            long long ttlMs = 24ll * 60 * 60 * 1000; // 24 hours
            long long expMs = nowMs + ttlMs;
            // remove any existing row for this client (optional)
            db->deleteData("client_tokens", "client_id = '" + clientID + "'");
            db->insertData("client_tokens",
                { "client_id", "token", "issued_at_ms", "expires_at_ms", "last_used_ms", "revoked", "revoked_at_ms" },
                { clientID, token, std::to_string(nowMs), std::to_string(expMs), std::to_string(nowMs), "0", "0" });
            nlohmann::json j;
            j["success"] = true;
            j["message"] = "Authorized";
            j["auth_code"] = token; // Client should send this value in the Authorization header for private endpoints
            res.set_content(j.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "authHeader",
        nlohmann::json({ { "type", "object" }, { "properties", { { "client_id", { { "type", "string" } } }, { "initial_code", { { "type", "string" } } } } }, { "required", nlohmann::json::array({ "client_id", "initial_code" }) } }),
        "Authorize the client. Returns an authentication header." });
    // Revoke token
    data.push_back({
        PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req,
        httplib::Response& res) {
            try {
                auto j = nlohmann::json::parse(req.body);
                auto clientID = j.value("client_id", std::string(""));
                auto token = j.value("token", std::string(""));
                Database* db = CubeDB::getDBManager()->getDatabase("auth");
                if (!db->isOpen()) throw std::runtime_error("Database not open");
                ensureTokenTable();
                auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                if (!token.empty()) {
                    db->updateData("client_tokens", { "revoked", "revoked_at_ms" }, { "1", std::to_string(nowMs) }, "token = '" + token + "'");
                }
                if (!clientID.empty()) {
                    // clear current client token and mark all tokens for this client revoked
                    db->updateData(DB_NS::TableNames::CLIENTS, { "auth_code" }, { "" }, "client_id = '" + clientID + "'");
                    db->updateData("client_tokens", { "revoked", "revoked_at_ms" }, { "1", std::to_string(nowMs) }, "client_id = '" + clientID + "'");
                }
                nlohmann::json out; out["success"] = true;
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            } catch (std::exception& e) {
                nlohmann::json out; out["success"] = false; out["message"] = e.what();
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
            }
        },
        "revokeToken",
        nlohmann::json({ { "type", "object" }, { "properties", { { "client_id", { { "type", "string" } } }, { "token", { { "type", "string" } } } } } }),
        "Revoke a token by client_id or token"
    });

    // Rotate token (issue new, revoke old)
    data.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                auto j = nlohmann::json::parse(req.body);
                auto clientID = j.at("client_id").get<std::string>();
                Database* db = CubeDB::getDBManager()->getDatabase("auth");
                if (!db->isOpen()) throw std::runtime_error("Database not open");
                ensureTokenTable();
                // revoke any existing tokens for this client
                auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                db->updateData("client_tokens", { "revoked", "revoked_at_ms" }, { "1", std::to_string(nowMs) }, "client_id = '" + clientID + "'");
                // issue new token
                std::string randomString = KeyGenerator(40);
                std::string encryptedString = CubeAuth::encryptData(randomString, CubeAuth::publicKey);
                std::string token = base64_encode_cube(encryptedString);
                long long ttlMs = 24ll * 60 * 60 * 1000; // 24 hours
                long long expMs = nowMs + ttlMs;
                db->updateData(DB_NS::TableNames::CLIENTS, { "auth_code" }, { token }, "client_id = '" + clientID + "'");
                db->insertData("client_tokens",
                    { "client_id", "token", "issued_at_ms", "expires_at_ms", "last_used_ms", "revoked", "revoked_at_ms" },
                    { clientID, token, std::to_string(nowMs), std::to_string(expMs), std::to_string(nowMs), "0", "0" });
                nlohmann::json out; out["success"] = true; out["token"] = token;
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            } catch (std::exception& e) {
                nlohmann::json out; out["success"] = false; out["message"] = e.what();
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
            }
        },
        "rotateToken",
        nlohmann::json({ { "type", "object" }, { "properties", { { "client_id", { { "type", "string" } } } } }, { "required", nlohmann::json::array({ "client_id" }) } }),
        "Rotate token for a client_id" });
    data.push_back({ PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            // get the appID from the query string
            std::string clientID = req.get_param_value("client_id");
            CubeLog::debug("GET initCode called with client_id: " + clientID);
            bool returnCode = req.has_param("return_code");
            // Get an initial code
            std::string initialCode = Code6Generator();
            CubeLog::debug("Generated initial code: " + initialCode);
            // then we store this in the db
            Database* db = CubeDB::getDBManager()->getDatabase("auth");
            if (!db->isOpen()) {
                CubeLog::error("Database not open.");
                nlohmann::json j;
                j["success"] = false;
                j["message"] = "Database not open.";
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Database not open.");
            }
            // check to see if the client_id exists
            if (!db->rowExists(DB_NS::TableNames::CLIENTS, "client_id = '" + clientID + "'")) {
                CubeLog::error("Client id not found. Adding client id.");
                if (!db->insertData(DB_NS::TableNames::CLIENTS, { "client_id", "initial_code", "auth_code", "role" }, { clientID, initialCode, "", "1" })) {
                    CubeLog::error("Failed to add client id.");
                    nlohmann::json j;
                    j["success"] = false;
                    j["message"] = "Failed to add client id.";
                    res.set_content(j.dump(), "application/json");
                    return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Failed to add client id.");
                }
            } else {
                // set the initial code in the db
                if (!db->updateData(DB_NS::TableNames::CLIENTS, { "initial_code" }, { initialCode }, "client_id = '" + clientID + "'")) {
                    CubeLog::error("Failed to set initial code.");
                    nlohmann::json j;
                    j["success"] = false;
                    j["message"] = "Failed to set initial code.";
                    res.set_content(j.dump(), "application/json");
                    return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Failed to set initial code.");
                }
            }
            nlohmann::json j;
            j["success"] = true;
            j["message"] = "Initial code generated";
            if (returnCode) {
                j["initial_code"] = initialCode;
            }
            res.set_content(j.dump(), "application/json");
            if (returnCode) {
                // Display the code on the device and allow confirmation (tap to approve, ignore to deny)
                // Include the client ID so the user can identify the requester
                GUI::showNotificationWithCallback(
                    "Authorization Request",
                    std::string("Client: ") + clientID + "\nCode: " + initialCode + "\nTap to approve, or ignore to deny.",
                    NotificationsManager::NotificationType::NOTIFICATION_YES_NO,
                    []() {},
                    [clientID]() {
                        Database* dbInner = CubeDB::getDBManager()->getDatabase("auth");
                        if (dbInner->isOpen()) {
                            dbInner->updateData(DB_NS::TableNames::CLIENTS, { "initial_code" }, { "" },
                                "client_id = '" + clientID + "'");
                        }
                    });
            } else {
                // Manual entry: show code only on device
                GUI::showMessageBox("Authorization Code", "Your authorization code is:\n" + initialCode);
            }
            std::thread([clientID, returnCode]() {
                std::this_thread::sleep_for(std::chrono::seconds(60));
                Database* dbInner = CubeDB::getDBManager()->getDatabase("auth");
                if (dbInner->isOpen()) {
                    dbInner->updateData(DB_NS::TableNames::CLIENTS, { "initial_code" }, { "" },
                        "client_id = '" + clientID + "'");
                }
                if (!returnCode) {
                    GUI::hideMessageBox();
                }
            }).detach();
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "initCode", 
        nlohmann::json({ { "type", "object" }, { "properties", { } } }), 
        "Generate an initial code for the client." });
    return data;
}

/**
 * @brief Get the HTTP endpoint names and parameters
 *
 * @return std::vector<std::pair<std::string, std::vector<std::string>>>
 */
// std::vector<std::pair<std::string, std::vector<std::string>>> CubeAuth::getHttpEndpointNamesAndParams()
// {
//     std::vector<std::pair<std::string, std::vector<std::string>>> namesAndParams;
//     namesAndParams.push_back({ "authHeader", { "client_id", "initial_code" } });
//     namesAndParams.push_back({ "initCode", { "client_id" } });
//     return namesAndParams;
// }

//////////////////////////////////////////////////////////////////////////////////////////////
