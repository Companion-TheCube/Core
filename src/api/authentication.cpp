#include "authentication.h"

std::string KeyGenerator(size_t length)
{
    std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string result;
    result.resize(length);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, charset.length() - 1);
    for (size_t i = 0; i < length; i++) {
        result[i] = charset[dis(gen)];
    }
    return result;
}

std::string Code6Generator()
{
    std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890123456789+=&%$#@!";
    std::string result;
    result.resize(6);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, charset.length() - 1);
    for (size_t i = 0; i < 6; i++) {
        result[i] = charset[dis(gen)];
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////

bool CubeAuth::available = false;
std::string CubeAuth::privateKey = "";
std::string CubeAuth::publicKey = "";

CubeAuth::CubeAuth()
{
    CubeLog::info("Authentication module starting...");
    if(sodium_init() == -1){
        CubeLog::error("Failed to initialize libsodium.");
        return;
    }
    std::pair<std::string,std::string> keyPair = generateKeyPair();
    CubeAuth::privateKey = keyPair.second;
    CubeAuth::publicKey = keyPair.first;
    CubeAuth::available = true;
}

CubeAuth::~CubeAuth()
{
    CubeLog::log("Authentication module stopping...", true);
}

int CubeAuth::checkAuth(std::string privateKey, std::string app_id, std::string encrypted_auth_code)
{
    if(!CubeAuth::available){
        CubeLog::error("Authentication module not available.");
        return CubeAuth::AUTH_FAIL_UNKNOWN;
    }
    if(privateKey.length() != 64){
        CubeLog::error("Invalid private key length.");
        return CubeAuth::AUTH_FAIL_INVALID_PRIVATE_KEY;
    }
    if(app_id.length() != CUBE_APPS_ID_LENGTH){
        CubeLog::error("Invalid app id length.");
        return CubeAuth::AUTH_FAIL_INVALID_APP_ID_LEN;
    }
    Database* db = CubeDB::DBManager()->getDatabase("auth");
    if(!db->isOpen()){
        CubeLog::error("Database not open.");
        return CubeAuth::AUTH_FAIL_DB_NOT_OPEN;
    }
    // check to see if the app_id exists
    if(!db->rowExists(DB_NS::TableNames::APPS, "app_id = '" + app_id + "'")){
        CubeLog::error("App id not found.");
        return CubeAuth::AUTH_FAIL_INVALID_APP_ID;
    }
    // get the public key
    std::vector<std::vector<std::string>> pub_key = db->selectData(DB_NS::TableNames::APPS, {"public_key"}, "app_id = '" + app_id + "'");
    // get the auth_code
    std::vector<std::vector<std::string>> auth_code = db->selectData(DB_NS::TableNames::APPS, {"auth_code"}, "app_id = '" + app_id + "'");
    //decrypt the encrypted_auth_code
    unsigned char decrypted_auth_code[crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES + 6];
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    unsigned char key[crypto_secretbox_KEYBYTES];
    unsigned char* encrypted_auth_code_ptr = (unsigned char*)encrypted_auth_code.c_str();
    unsigned char* public_key_ptr = (unsigned char*)pub_key[0][0].c_str();
    unsigned char* private_key_ptr = (unsigned char*)privateKey.c_str();
    if(crypto_box_beforenm(key, public_key_ptr, private_key_ptr) != 0){
        CubeLog::error("Failed to generate key.");
        return CubeAuth::AUTH_FAIL_KEY_GEN;
    }
    memcpy(nonce, encrypted_auth_code_ptr, crypto_secretbox_NONCEBYTES);
    memcpy(decrypted_auth_code, encrypted_auth_code_ptr + crypto_secretbox_NONCEBYTES, crypto_secretbox_MACBYTES + 6);
    if(crypto_secretbox_open_easy(decrypted_auth_code + crypto_secretbox_MACBYTES, decrypted_auth_code, crypto_secretbox_MACBYTES + 6, nonce, key) != 0){
        CubeLog::error("Failed to decrypt auth code.");
        return CubeAuth::AUTH_FAIL_DECRYPT;
    }
    std::string decrypted_auth_code_str((char*)decrypted_auth_code + crypto_secretbox_MACBYTES);
    // compare the decrypted auth code with the auth code in the database
    if(decrypted_auth_code_str != auth_code[0][0]){
        CubeLog::error("Auth code mismatch.");
        return CubeAuth::AUTH_FAIL_CODE_MISMATCH;
    }
    return CubeAuth::AUTH_SUCCESS;
}

std::string CubeAuth::generateAuthCode()
{
    return Code6Generator();
}

std::pair<std::string,std::string> CubeAuth::generateKeyPair()
{
    std::string public_key(crypto_box_PUBLICKEYBYTES, 0);
    std::string private_key(crypto_box_SECRETKEYBYTES, 0);
    crypto_box_keypair((unsigned char*)public_key.c_str(), (unsigned char*)private_key.c_str());
    return std::make_pair(public_key, private_key);
}

std::string CubeAuth::encryptAuthCode(std::string auth_code, std::string public_key)
{
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    unsigned char key[crypto_secretbox_KEYBYTES];
    unsigned char encrypted_auth_code[crypto_secretbox_MACBYTES + 6];
    unsigned char* auth_code_ptr = (unsigned char*)auth_code.c_str();
    unsigned char* public_key_ptr = (unsigned char*)public_key.c_str();
    unsigned char* private_key_ptr = (unsigned char*)CubeAuth::privateKey.c_str();
    if(crypto_box_beforenm(key, public_key_ptr, private_key_ptr) != 0){
        CubeLog::error("Failed to generate key.");
        return "";
    }
    randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
    if(crypto_secretbox_easy(encrypted_auth_code, auth_code_ptr, 6, nonce, key) != 0){
        CubeLog::error("Failed to encrypt auth code.");
        return "";
    }
    std::string encrypted_auth_code_str((char*)nonce, crypto_secretbox_NONCEBYTES);
    encrypted_auth_code_str.append((char*)encrypted_auth_code, crypto_secretbox_MACBYTES + 6);
    return encrypted_auth_code_str;
}

std::string CubeAuth::decryptAuthCode(std::string auth_code, std::string private_key)
{
    unsigned char decrypted_auth_code[6];
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    unsigned char key[crypto_secretbox_KEYBYTES];
    unsigned char* encrypted_auth_code_ptr = (unsigned char*)auth_code.c_str();
    unsigned char* private_key_ptr = (unsigned char*)private_key.c_str();
    unsigned char* public_key_ptr = (unsigned char*)CubeAuth::publicKey.c_str();
    if(crypto_box_beforenm(key, public_key_ptr, private_key_ptr) != 0){
        CubeLog::error("Failed to generate key.");
        return "";
    }
    memcpy(nonce, encrypted_auth_code_ptr, crypto_secretbox_NONCEBYTES);
    if(crypto_secretbox_open_easy(decrypted_auth_code, encrypted_auth_code_ptr + crypto_secretbox_NONCEBYTES, crypto_secretbox_MACBYTES + 6, nonce, key) != 0){
        CubeLog::error("Failed to decrypt auth code.");
        return "";
    }
    return std::string((char*)decrypted_auth_code, 6);
}

std::string CubeAuth::encryptData(std::string data, std::string public_key)
{
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    unsigned char key[crypto_secretbox_KEYBYTES];
    unsigned char* data_ptr = (unsigned char*)data.c_str();
    unsigned char* public_key_ptr = (unsigned char*)public_key.c_str();
    unsigned char* private_key_ptr = (unsigned char*)CubeAuth::privateKey.c_str();
    if(crypto_box_beforenm(key, public_key_ptr, private_key_ptr) != 0){
        CubeLog::error("Failed to generate key.");
        return "";
    }
    randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
    std::string encrypted_data(crypto_secretbox_MACBYTES + data.length(), 0);
    if(crypto_secretbox_easy((unsigned char*)encrypted_data.c_str(), data_ptr, data.length(), nonce, key) != 0){
        CubeLog::error("Failed to encrypt data.");
        return "";
    }
    std::string encrypted_data_str((char*)nonce, crypto_secretbox_NONCEBYTES);
    encrypted_data_str.append(encrypted_data);
    return encrypted_data_str;
}

std::string CubeAuth::decryptData(std::string data, std::string private_key, size_t length)
{
    unsigned char* decrypted_data = new unsigned char[length];
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    unsigned char key[crypto_secretbox_KEYBYTES];
    unsigned char* encrypted_data_ptr = (unsigned char*)data.c_str();
    unsigned char* private_key_ptr = (unsigned char*)private_key.c_str();
    unsigned char* public_key_ptr = (unsigned char*)CubeAuth::publicKey.c_str();
    if(crypto_box_beforenm(key, public_key_ptr, private_key_ptr) != 0){
        CubeLog::error("Failed to generate key.");
        return "";
    }
    memcpy(nonce, encrypted_data_ptr, crypto_secretbox_NONCEBYTES);
    if(crypto_secretbox_open_easy(decrypted_data, encrypted_data_ptr + crypto_secretbox_NONCEBYTES, length, nonce, key) != 0){
        CubeLog::error("Failed to decrypt data.");
        return "";
    }
    std::string decrypted_data_str((char*)decrypted_data, length);
    delete[] decrypted_data;
    return decrypted_data_str;
}
//////////////////////////////////////////////////////////////////////////////////////////////