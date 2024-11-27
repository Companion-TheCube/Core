#ifndef LOGGER_H
#include <logger.h>
#endif
#include "authentication.h"

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
 * @brief Generate a random 6 character code using the charset of 0-9, A-Z, a-z, and special characters
 *
 * @return std::string
 */
std::string Code6Generator()
{
    CubeLog::debug("Generating 6 character code");
    std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890123456789+=&%$#@!";
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
    Database* db = CubeDB::getDBManager()->getDatabase("auth");
    if (!db->isOpen()) {
        CubeLog::error("Database not open.");
        CubeAuth::lastError = "Database not open.";
        return false;
    }
    // get the auth code from the db
    std::vector<std::vector<std::string>> auth_code = db->selectData(DB_NS::TableNames::CLIENTS, { "auth_code" }, "auth_code = '" + authHeader + "'");
    if (auth_code.size() == 0) {
        CubeLog::error("Auth code not found.");
        CubeAuth::lastError = "Auth code not found.";
        return false;
    }
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
            // set the auth code in the db
            if (!db->updateData(DB_NS::TableNames::CLIENTS, { "auth_code" }, { randomString }, "client_id = '" + clientID + "'")) {
                CubeLog::error("Failed to set auth code.");
                nlohmann::json j;
                j["success"] = false;
                j["message"] = "Failed to set auth code.";
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Failed to set auth code.");
            }
            nlohmann::json j;
            j["success"] = true;
            j["message"] = "Authorized";
            j["auth_code"] = encryptedString;
            res.set_content(j.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "authHeader", { "client_id", "initial_code" }, "Authorize the client. Returns an authentication header." });
    data.push_back({ PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            // get the appID from the query string
            std::string clientID = req.get_param_value("client_id");
            CubeLog::debug("GET initCode called with client_id: " + clientID);
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
            // We don't send the initial code back to the client since the user has to read it form the screen and enter it into the client
            res.set_content(j.dump(), "application/json");
            // we need to display the initial code to the user
            GUI::showMessageBox("Authorization Code", "Your authorization code is:\n" + initialCode); // TODO: verify/test that this is thread safe
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "initCode", { "client_id" }, "Generate an initial code for the client." });
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