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
#include <algorithm>
#include <cctype>
#include <chrono>
#include <optional>
#include <thread>

namespace {
constexpr const char* kClientAuthRequestsTable = "client_auth_requests";
constexpr const char* kAuthorizedEndpointsTable = "authorized_endpoints";
constexpr const char* kAuthRequestStatusPending = "pending";
constexpr const char* kAuthRequestStatusApproved = "approved";
constexpr const char* kAuthRequestStatusDenied = "denied";
constexpr const char* kAuthRequestStatusExpired = "expired";
constexpr const char* kAuthRequestStatusConsumed = "consumed";
constexpr long long kAuthRequestTtlMs = 60ll * 1000ll;

struct AuthRequestRecord {
    std::string clientId;
    std::string initialCode;
    std::string status;
    long long requestedAtMs = 0;
    long long expiresAtMs = 0;
    long long approvedAtMs = 0;
    long long deniedAtMs = 0;
    long long consumedAtMs = 0;
};

long long nowEpochMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch())
        .count();
}

long long parseLongLongOrZero(const std::string& value)
{
    try {
        return std::stoll(value);
    } catch (...) {
        return 0;
    }
}

bool isTruthyConfigValue(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value == "1" || value == "true" || value == "yes" || value == "on";
}

bool configFlagEnabled(const std::string& key)
{
    return isTruthyConfigValue(Config::get(key, "0"));
}

void respondJson(httplib::Response& res, const nlohmann::json& body, int status = 200)
{
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

nlohmann::json authFailureJson(const std::string& code, const std::string& message)
{
    return nlohmann::json {
        { "success", false },
        { "error", code },
        { "message", message }
    };
}

DB_NS::PredicateList clientIdFilter(const std::string& clientID)
{
    return { DB_NS::Predicate { "client_id", clientID } };
}

DB_NS::PredicateList tokenFilter(const std::string& token)
{
    return { DB_NS::Predicate { "token", token } };
}

DB_NS::PredicateList appIdFilter(const std::string& appID)
{
    return { DB_NS::Predicate { "app_id", appID } };
}

DB_NS::PredicateList appAuthIdFilter(const std::string& appAuthId)
{
    return { DB_NS::Predicate { "app_auth_id", appAuthId } };
}

DB_NS::PredicateList authorizedEndpointFilter(const std::string& appID, const std::string& endpointName)
{
    return {
        DB_NS::Predicate { "app_id", appID },
        DB_NS::Predicate { "endpoint_name", endpointName }
    };
}

DB_NS::PredicateList authRequestKeyFilter(const std::string& clientID, const std::string& initialCode)
{
    return {
        DB_NS::Predicate { "client_id", clientID },
        DB_NS::Predicate { "initial_code", initialCode }
    };
}

bool ensureAuthRequestTable(Database* db)
{
    if (db == nullptr || !db->isOpen()) {
        return false;
    }
    if (!db->tableExists(kClientAuthRequestsTable)) {
        return db->createTable(kClientAuthRequestsTable,
            { "id", "client_id", "initial_code", "status", "requested_at_ms", "expires_at_ms", "approved_at_ms", "denied_at_ms", "consumed_at_ms" },
            { "INTEGER PRIMARY KEY", "TEXT", "TEXT", "TEXT", "INTEGER", "INTEGER", "INTEGER", "INTEGER", "INTEGER" },
            { true, true, false, false, false, false, false, false, false });
    }
    return true;
}

bool ensureAuthorizedEndpointsTable(Database* db, std::string& error)
{
    if (db == nullptr || !db->isOpen()) {
        error = "Apps database not open.";
        return false;
    }
    if (db->tableExists(kAuthorizedEndpointsTable)) {
        return true;
    }
    if (!db->createTable(kAuthorizedEndpointsTable,
            { "id", "app_id", "endpoint_name" },
            { "INTEGER PRIMARY KEY", "TEXT", "TEXT" },
            { true, false, false })) {
        error = "Failed to create authorized_endpoints table: " + db->getLastError();
        return false;
    }
    return true;
}

bool resolveAppAuthId(const std::string& appAuthId, std::string& appIdOut, std::string& error)
{
    if (appAuthId.empty()) {
        error = "App auth ID is required.";
        return false;
    }

    auto* db = CubeDB::getDBManager()->getDatabase("apps");
    if (db == nullptr || !db->isOpen()) {
        error = "Apps database not open.";
        return false;
    }

    const auto rows = db->selectData(DB_NS::TableNames::APPS, { "app_id", "enabled" }, appAuthIdFilter(appAuthId));
    if (rows.empty()) {
        error = "App auth ID not found.";
        return false;
    }
    if (rows.size() != 1 || rows.front().size() < 2) {
        error = "App auth ID resolved ambiguously.";
        return false;
    }
    if (rows.front()[1] != "1") {
        error = "App is disabled.";
        return false;
    }

    appIdOut = rows.front()[0];
    return true;
}

std::optional<AuthRequestRecord> getAuthRequest(Database* db, const std::string& clientID)
{
    if (!ensureAuthRequestTable(db)) {
        return std::nullopt;
    }
    const auto rows = db->selectData(kClientAuthRequestsTable,
        { "client_id", "initial_code", "status", "requested_at_ms", "expires_at_ms", "approved_at_ms", "denied_at_ms", "consumed_at_ms" },
        clientIdFilter(clientID));
    if (rows.empty() || rows.front().size() < 8) {
        return std::nullopt;
    }

    const auto& row = rows.front();
    return AuthRequestRecord {
        row[0],
        row[1],
        row[2],
        parseLongLongOrZero(row[3]),
        parseLongLongOrZero(row[4]),
        parseLongLongOrZero(row[5]),
        parseLongLongOrZero(row[6]),
        parseLongLongOrZero(row[7])
    };
}

bool upsertPendingAuthRequest(Database* db,
    const std::string& clientID,
    const std::string& initialCode,
    long long requestedAtMs,
    long long expiresAtMs)
{
    if (!ensureAuthRequestTable(db)) {
        return false;
    }
    if (!db->deleteData(kClientAuthRequestsTable, clientIdFilter(clientID))) {
        return false;
    }
    return -1 < db->insertData(kClientAuthRequestsTable,
        { "client_id", "initial_code", "status", "requested_at_ms", "expires_at_ms", "approved_at_ms", "denied_at_ms", "consumed_at_ms" },
        { clientID, initialCode, kAuthRequestStatusPending, std::to_string(requestedAtMs), std::to_string(expiresAtMs), "0", "0", "0" });
}

bool approveAuthRequest(Database* db, const std::string& clientID, const std::string& initialCode, long long approvedAtMs)
{
    if (!ensureAuthRequestTable(db)) {
        return false;
    }
    if (!db->updateData(kClientAuthRequestsTable,
            { "status", "approved_at_ms" },
            { kAuthRequestStatusApproved, std::to_string(approvedAtMs) },
            {
                DB_NS::Predicate { "client_id", clientID },
                DB_NS::Predicate { "initial_code", initialCode },
                DB_NS::Predicate { "status", kAuthRequestStatusPending },
                DB_NS::Predicate { "consumed_at_ms", "0" }
            })) {
        return false;
    }
    const auto request = getAuthRequest(db, clientID);
    return request.has_value()
        && request->initialCode == initialCode
        && request->status == kAuthRequestStatusApproved;
}

bool denyAuthRequest(Database* db, const std::string& clientID, const std::string& initialCode, long long deniedAtMs)
{
    if (!ensureAuthRequestTable(db)) {
        return false;
    }
    if (!db->updateData(kClientAuthRequestsTable,
            { "status", "denied_at_ms" },
            { kAuthRequestStatusDenied, std::to_string(deniedAtMs) },
            {
                DB_NS::Predicate { "client_id", clientID },
                DB_NS::Predicate { "initial_code", initialCode },
                DB_NS::Predicate { "status", kAuthRequestStatusPending },
                DB_NS::Predicate { "consumed_at_ms", "0" }
            })) {
        return false;
    }
    const auto request = getAuthRequest(db, clientID);
    return request.has_value()
        && request->initialCode == initialCode
        && request->status == kAuthRequestStatusDenied;
}

bool expireAuthRequestIfOutstanding(Database* db, const std::string& clientID, const std::string& initialCode)
{
    if (!ensureAuthRequestTable(db)) {
        return false;
    }
    const auto request = getAuthRequest(db, clientID);
    if (!request.has_value() || request->initialCode != initialCode || request->consumedAtMs > 0) {
        return false;
    }
    if (request->status != kAuthRequestStatusPending && request->status != kAuthRequestStatusApproved) {
        return request->status == kAuthRequestStatusExpired;
    }
    if (!db->updateData(kClientAuthRequestsTable,
            { "status" },
            { kAuthRequestStatusExpired },
            {
                DB_NS::Predicate { "client_id", clientID },
                DB_NS::Predicate { "initial_code", initialCode },
                DB_NS::Predicate { "status", request->status },
                DB_NS::Predicate { "consumed_at_ms", "0" }
            })) {
        return false;
    }
    const auto updatedRequest = getAuthRequest(db, clientID);
    return updatedRequest.has_value()
        && updatedRequest->initialCode == initialCode
        && updatedRequest->status == kAuthRequestStatusExpired;
}

bool consumeApprovedAuthRequest(Database* db, const std::string& clientID, const std::string& initialCode, long long consumedAtMs)
{
    if (!ensureAuthRequestTable(db)) {
        return false;
    }
    if (!db->updateData(kClientAuthRequestsTable,
            { "status", "consumed_at_ms" },
            { kAuthRequestStatusConsumed, std::to_string(consumedAtMs) },
            {
                DB_NS::Predicate { "client_id", clientID },
                DB_NS::Predicate { "initial_code", initialCode },
                DB_NS::Predicate { "status", kAuthRequestStatusApproved },
                DB_NS::Predicate { "consumed_at_ms", "0" }
            })) {
        return false;
    }
    const auto request = getAuthRequest(db, clientID);
    return request.has_value()
        && request->initialCode == initialCode
        && request->status == kAuthRequestStatusConsumed
        && request->consumedAtMs == consumedAtMs;
}

bool storeClientToken(Database* db, const std::string& clientID, const std::string& token, long long issuedAtMs)
{
    if (db == nullptr || !db->isOpen()) {
        return false;
    }

    const auto clientWhere = clientIdFilter(clientID);
    if (db->rowExists(DB_NS::TableNames::CLIENTS, clientWhere)) {
        if (!db->updateData(DB_NS::TableNames::CLIENTS,
                { "initial_code", "auth_code" },
                { "", token },
                clientWhere)) {
            return false;
        }
    } else if (-1 >= db->insertData(DB_NS::TableNames::CLIENTS,
                   { "client_id", "initial_code", "auth_code", "role" },
                   { clientID, "", token, "1" })) {
        return false;
    }

    if (!CubeAuth::ensureTokenTable()) {
        return false;
    }

    const long long expiresAtMs = issuedAtMs + (24ll * 60ll * 60ll * 1000ll);
    if (!db->deleteData("client_tokens", clientIdFilter(clientID))) {
        return false;
    }
    return -1 < db->insertData("client_tokens",
        { "client_id", "token", "issued_at_ms", "expires_at_ms", "last_used_ms", "revoked", "revoked_at_ms" },
        { clientID, token, std::to_string(issuedAtMs), std::to_string(expiresAtMs), std::to_string(issuedAtMs), "0", "0" });
}

void clearClientToken(Database* db, const std::string& clientID)
{
    if (db == nullptr || !db->isOpen()) {
        return;
    }
    db->updateData(DB_NS::TableNames::CLIENTS,
        { "auth_code" },
        { "" },
        clientIdFilter(clientID));
    if (CubeAuth::ensureTokenTable()) {
        db->deleteData("client_tokens", clientIdFilter(clientID));
    }
}
} // namespace

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
    if (!db->rowExists(DB_NS::TableNames::APPS, appIdFilter(app_id))) {
        CubeLog::error("App id not found.");
        CubeAuth::lastError = "App id not found.";
        return AUTH_CODE::AUTH_FAIL_INVALID_APP_ID;
    }
    // get the public key
    std::vector<std::vector<std::string>> pub_key = db->selectData(DB_NS::TableNames::APPS, { "public_key" }, appIdFilter(app_id));
    // get the auth_code
    std::vector<std::vector<std::string>> auth_code = db->selectData(DB_NS::TableNames::APPS, { "auth_code" }, appIdFilter(app_id));
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

bool CubeAuth::hasValidAppIdentity(const std::string& appAuthId)
{
    std::string resolvedAppId;
    std::string error;
    const bool resolved = resolveAppAuthId(appAuthId, resolvedAppId, error);
    CubeAuth::lastError = resolved ? "" : error;
    return resolved;
}

bool CubeAuth::isAuthorizedApp(const std::string& appAuthId, const std::string& endpointName)
{
    if (endpointName.empty()) {
        CubeAuth::lastError = "Endpoint name is required.";
        return false;
    }

    std::string resolvedAppId;
    std::string error;
    if (!resolveAppAuthId(appAuthId, resolvedAppId, error)) {
        CubeAuth::lastError = error;
        return false;
    }

    auto* db = CubeDB::getDBManager()->getDatabase("apps");
    if (db == nullptr || !db->isOpen()) {
        CubeAuth::lastError = "Apps database not open.";
        return false;
    }
    if (!ensureAuthorizedEndpointsTable(db, error)) {
        CubeAuth::lastError = error;
        return false;
    }
    if (!db->rowExists(kAuthorizedEndpointsTable, authorizedEndpointFilter(resolvedAppId, endpointName))) {
        CubeAuth::lastError = "App is not authorized to access endpoint.";
        return false;
    }

    CubeAuth::lastError = "";
    return true;
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
    auto auth_code = db->selectData(DB_NS::TableNames::CLIENTS, { "client_id", "auth_code" }, { DB_NS::Predicate { "auth_code", token } });
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
    auto tokenRows = db->selectData("client_tokens", { "revoked", "expires_at_ms" }, tokenFilter(token));
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
    db->updateData("client_tokens", { "last_used_ms" }, { std::to_string(nowMs) }, tokenFilter(token));
    return true;
}

/**
 * @brief Get the name of the interface
 *
 * @return std::string
 */
std::string CubeAuth::getInterfaceName() const
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
            const std::string clientID = req.get_param_value("client_id");
            const std::string initialCode = req.get_param_value("initial_code");
            CubeLog::debug("GET authHeader called with client_id: " + clientID);
            if (clientID.empty() || initialCode.empty()) {
                respondJson(res,
                    authFailureJson("invalid_request", "client_id and initial_code are required."),
                    httplib::StatusCode::BadRequest_400);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }

            Database* db = CubeDB::getDBManager()->getDatabase("auth");
            if (!db->isOpen() || !ensureAuthRequestTable(db)) {
                CubeLog::error("Database not open.");
                respondJson(res,
                    authFailureJson("internal_error", "Database not open."),
                    httplib::StatusCode::InternalServerError_500);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }

            auto request = getAuthRequest(db, clientID);
            if (!request.has_value() || request->initialCode != initialCode) {
                CubeLog::warning("Auth request not found or code mismatch for client_id: " + clientID);
                respondJson(res,
                    authFailureJson("invalid_code", "Approval code is invalid."),
                    httplib::StatusCode::Forbidden_403);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }

            const auto currentTimeMs = nowEpochMs();
            if (request->consumedAtMs > 0 || request->status == kAuthRequestStatusConsumed) {
                respondJson(res,
                    authFailureJson("approval_consumed", "Approval code has already been used."),
                    httplib::StatusCode::Forbidden_403);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }
            if (request->status == kAuthRequestStatusDenied) {
                respondJson(res,
                    authFailureJson("approval_denied", "Approval request was denied."),
                    httplib::StatusCode::Forbidden_403);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }

            if (request->status == kAuthRequestStatusExpired
                || (request->expiresAtMs > 0 && currentTimeMs > request->expiresAtMs)) {
                expireAuthRequestIfOutstanding(db, clientID, initialCode);
                respondJson(res,
                    authFailureJson("approval_expired", "Approval request has expired."),
                    httplib::StatusCode::Forbidden_403);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }

            if (request->status != kAuthRequestStatusApproved) {
                respondJson(res,
                    authFailureJson("approval_pending", "Approval request is still pending."),
                    httplib::StatusCode::Forbidden_403);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }

            std::string randomString = KeyGenerator(30 + (rand() % 21));
            std::string encryptedString = CubeAuth::encryptData(randomString, CubeAuth::publicKey);
            CubeLog::debug("Generated auth token payload for client_id: " + clientID);
            if (encryptedString.empty()) {
                CubeLog::error("Failed to encrypt auth token payload.");
                respondJson(res,
                    authFailureJson("internal_error", CubeAuth::getLastError()),
                    httplib::StatusCode::InternalServerError_500);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }

            std::string token = base64_encode_cube(encryptedString);
            if (!storeClientToken(db, clientID, token, currentTimeMs)) {
                CubeLog::error("Failed to store auth token for client_id: " + clientID);
                respondJson(res,
                    authFailureJson("internal_error", "Failed to store auth token."),
                    httplib::StatusCode::InternalServerError_500);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }

            if (!consumeApprovedAuthRequest(db, clientID, initialCode, currentTimeMs)) {
                clearClientToken(db, clientID);
                CubeLog::warning("Approval code was consumed concurrently for client_id: " + clientID);
                respondJson(res,
                    authFailureJson("approval_consumed", "Approval code has already been used."),
                    httplib::StatusCode::Forbidden_403);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }

            nlohmann::json j;
            j["success"] = true;
            j["message"] = "Authorized";
            j["auth_code"] = token;
            respondJson(res, j);
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "authHeader",
        nlohmann::json({ { "type", "object" }, { "properties", { { "client_id", { { "type", "string" } } }, { "initial_code", { { "type", "string" } } } } }, { "required", nlohmann::json::array({ "client_id", "initial_code" }) } }),
        "Authorize the client after device approval. Returns an authentication header." });
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
                auto nowMs = nowEpochMs();
                if (!token.empty()) {
                    db->updateData("client_tokens", { "revoked", "revoked_at_ms" }, { "1", std::to_string(nowMs) }, tokenFilter(token));
                }
                if (!clientID.empty()) {
                    // clear current client token and mark all tokens for this client revoked
                    db->updateData(DB_NS::TableNames::CLIENTS, { "auth_code" }, { "" }, clientIdFilter(clientID));
                    db->updateData("client_tokens", { "revoked", "revoked_at_ms" }, { "1", std::to_string(nowMs) }, clientIdFilter(clientID));
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
                auto nowMs = nowEpochMs();
                db->updateData("client_tokens", { "revoked", "revoked_at_ms" }, { "1", std::to_string(nowMs) }, clientIdFilter(clientID));
                // issue new token
                std::string randomString = KeyGenerator(40);
                std::string encryptedString = CubeAuth::encryptData(randomString, CubeAuth::publicKey);
                std::string token = base64_encode_cube(encryptedString);
                long long ttlMs = 24ll * 60 * 60 * 1000; // 24 hours
                long long expMs = nowMs + ttlMs;
                db->updateData(DB_NS::TableNames::CLIENTS, { "auth_code" }, { token }, clientIdFilter(clientID));
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
            std::string clientID = req.get_param_value("client_id");
            CubeLog::debug("GET initCode called with client_id: " + clientID);
            if (clientID.empty()) {
                respondJson(res,
                    authFailureJson("invalid_request", "client_id is required."),
                    httplib::StatusCode::BadRequest_400);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }

            const bool returnCodeRequested = req.has_param("return_code");
            const bool allowReturnCode = configFlagEnabled("AUTH_ALLOW_RETURN_CODE");
            const bool autoApproveRequests = configFlagEnabled("AUTH_AUTO_APPROVE_REQUESTS");
            std::string initialCode = Code6Generator();
            CubeLog::debug("Generated initial code: " + initialCode);
            Database* db = CubeDB::getDBManager()->getDatabase("auth");
            if (!db->isOpen() || !ensureAuthRequestTable(db)) {
                CubeLog::error("Database not open.");
                respondJson(res,
                    authFailureJson("internal_error", "Database not open."),
                    httplib::StatusCode::InternalServerError_500);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }

            const auto requestedAtMs = nowEpochMs();
            const auto expiresAtMs = requestedAtMs + kAuthRequestTtlMs;
            if (!upsertPendingAuthRequest(db, clientID, initialCode, requestedAtMs, expiresAtMs)) {
                CubeLog::error("Failed to create auth request for client id: " + clientID);
                respondJson(res,
                    authFailureJson("internal_error", "Failed to create auth request."),
                    httplib::StatusCode::InternalServerError_500);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }

            if (autoApproveRequests) {
                approveAuthRequest(db, clientID, initialCode, requestedAtMs);
            }

            nlohmann::json j;
            j["success"] = true;
            j["message"] = "Approval requested";
            if (returnCodeRequested && allowReturnCode) {
                j["initial_code"] = initialCode;
            }
            respondJson(res, j);

            GUI::showNotificationWithCallback(
                "Authorization Request",
                std::string("Client: ") + clientID + "\nCode: " + initialCode + "\nTap to approve, or deny to reject.",
                NotificationsManager::NotificationType::NOTIFICATION_YES_NO,
                [clientID, initialCode]() {
                    Database* dbInner = CubeDB::getDBManager()->getDatabase("auth");
                    if (dbInner != nullptr && dbInner->isOpen()) {
                        approveAuthRequest(dbInner, clientID, initialCode, nowEpochMs());
                    }
                },
                [clientID, initialCode]() {
                    Database* dbInner = CubeDB::getDBManager()->getDatabase("auth");
                    if (dbInner == nullptr || !dbInner->isOpen()) {
                        return;
                    }
                    const auto request = getAuthRequest(dbInner, clientID);
                    if (!request.has_value() || request->initialCode != initialCode) {
                        return;
                    }
                    const auto currentTimeMs = nowEpochMs();
                    if (request->status == kAuthRequestStatusExpired
                        || (request->expiresAtMs > 0 && currentTimeMs >= request->expiresAtMs)) {
                        expireAuthRequestIfOutstanding(dbInner, clientID, initialCode);
                    } else {
                        denyAuthRequest(dbInner, clientID, initialCode, currentTimeMs);
                    }
                });

            std::thread([clientID, initialCode]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(kAuthRequestTtlMs));
                Database* dbInner = CubeDB::getDBManager()->getDatabase("auth");
                if (dbInner != nullptr && dbInner->isOpen()) {
                    expireAuthRequestIfOutstanding(dbInner, clientID, initialCode);
                }
            }).detach();
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "initCode", 
        nlohmann::json({ { "type", "object" }, { "properties", { } } }), 
        "Request device approval and optionally return the initial code in dev/test mode." });
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
