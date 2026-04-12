/*
 █████╗ ██████╗ ██████╗ ██████╗  ██████╗ ███████╗████████╗ ██████╗ ██████╗ ███████╗███████╗ █████╗  ██████╗ ██████╗███████╗███████╗███████╗    ██████╗██████╗ ██████╗
██╔══██╗██╔══██╗██╔══██╗██╔══██╗██╔═══██╗██╔════╝╚══██╔══╝██╔════╝ ██╔══██╗██╔════╝██╔════╝██╔══██╗██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝   ██╔════╝██╔══██╗██╔══██╗
███████║██████╔╝██████╔╝██████╔╝██║   ██║███████╗   ██║   ██║  ███╗██████╔╝█████╗  ███████╗███████║██║     ██║     █████╗  ███████╗███████╗   ██║     ██████╔╝██████╔╝
██╔══██║██╔═══╝ ██╔═══╝ ██╔═══╝ ██║   ██║╚════██║   ██║   ██║   ██║██╔══██╗██╔══╝  ╚════██║██╔══██║██║     ██║     ██╔══╝  ╚════██║╚════██║   ██║     ██╔═══╝ ██╔═══╝
██║  ██║██║     ██║     ██║     ╚██████╔╝███████║   ██║   ╚██████╔╝██║  ██║███████╗███████║██║  ██║╚██████╗╚██████╗███████╗███████║███████║██╗╚██████╗██║     ██║
╚═╝  ╚═╝╚═╝     ╚═╝     ╚═╝      ╚═════╝ ╚══════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝╚══════╝╚══════╝╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC

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

#include "appPostgresAccess.h"

#include <pqxx/pqxx>
#include <sodium.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <optional>
#include <regex>
#include <set>
#include <sstream>

#ifndef LOGGER_H
#include <logger.h>
#endif

namespace {
namespace fs = std::filesystem;

constexpr const char* kAppsDbName = "apps";
constexpr const char* kAccessTableName = "app_postgresql_access";
constexpr const char* kDefaultPostgresAppId = "com.thecube.postgresql";
constexpr const char* kDefaultPostgresHost = "127.0.0.1";
constexpr int kDefaultPostgresPort = 5432;
constexpr const char* kDefaultPostgresAdminDb = "postgres";
constexpr const char* kDefaultPostgresAdminUser = "postgres";
constexpr const char* kDefaultPostgresAdminPassword = "thecube-postgres-admin";
constexpr const char* kDefaultSecretKeyPath = "data/apps_postgresql_access.key";
constexpr const char* kBootstrapHeaderName = "X-TheCube-App-Bootstrap-Token";
constexpr const char* kBootstrapEnvVarName = "THECUBE_POSTGRES_BOOTSTRAP_TOKEN";
constexpr const char* kEndpointEnvVarName = "THECUBE_POSTGRES_CREDENTIALS_ENDPOINT";
constexpr const char* kCredentialsEndpointPath = "/AppPostgresAccess-fetchCredentials";
const std::regex kLogicalDatabasePattern("^[a-z][a-z0-9_]*$");

struct AccessRow {
    std::string appId;
    std::string roleName;
    std::string encryptedPassword;
    nlohmann::json databaseMapping = nlohmann::json::object();
    std::string bootstrapTokenHash;
    std::string provisionedAtMs;
    std::string lastFetchAtMs;
};

std::mutex gAdminClientMutex;
std::shared_ptr<AppPostgresAdminClient> gAdminClientForTests;

int64_t epochMsNow()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

bool ensureSodiumReady(std::string& error)
{
    static const bool initialized = []() {
        return sodium_init() >= 0;
    }();
    if (!initialized) {
        error = "Failed to initialize libsodium";
        return false;
    }
    return true;
}

template <typename Transaction>
void execNoRows(Transaction& tx, const std::string& sql)
{
    // libpqxx 6.x uses exec0(); newer releases support result::no_rows().
    if constexpr (requires { tx.exec(sql).no_rows(); }) {
        tx.exec(sql).no_rows();
    } else {
        tx.exec0(sql);
    }
}

std::string hexEncode(const unsigned char* data, size_t size)
{
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(size * 2);
    for (size_t i = 0; i < size; ++i) {
        const unsigned char value = data[i];
        out.push_back(digits[(value >> 4) & 0x0F]);
        out.push_back(digits[value & 0x0F]);
    }
    return out;
}

std::vector<unsigned char> hexDecode(const std::string& input)
{
    if (input.size() % 2 != 0) {
        return {};
    }

    std::vector<unsigned char> out;
    out.reserve(input.size() / 2);
    for (size_t i = 0; i < input.size(); i += 2) {
        const auto byteString = input.substr(i, 2);
        try {
            out.push_back(static_cast<unsigned char>(std::stoul(byteString, nullptr, 16)));
        } catch (...) {
            return {};
        }
    }
    return out;
}

fs::path secretKeyPath()
{
    return fs::path(Config::get("APPS_POSTGRES_SECRET_KEY_PATH", kDefaultSecretKeyPath));
}

bool loadOrCreateLocalSecret(std::vector<unsigned char>& keyOut, std::string& secretHexOut, std::string& error)
{
    if (!ensureSodiumReady(error)) {
        return false;
    }

    const auto path = secretKeyPath();
    std::error_code ec;
    if (fs::exists(path, ec) && !ec) {
        std::ifstream input(path);
        if (!input.is_open()) {
            error = "Failed to open PostgreSQL secret key file: " + path.string();
            return false;
        }
        std::getline(input, secretHexOut);
        keyOut = hexDecode(secretHexOut);
        if (keyOut.size() != crypto_secretbox_KEYBYTES) {
            error = "Invalid PostgreSQL secret key file contents";
            return false;
        }
        return true;
    }

    keyOut.resize(crypto_secretbox_KEYBYTES);
    randombytes_buf(keyOut.data(), keyOut.size());
    secretHexOut = hexEncode(keyOut.data(), keyOut.size());

    if (!path.parent_path().empty()) {
        fs::create_directories(path.parent_path(), ec);
        if (ec) {
            error = "Failed to create PostgreSQL secret key directory: " + ec.message();
            return false;
        }
    }

    std::ofstream output(path);
    if (!output.is_open()) {
        error = "Failed to write PostgreSQL secret key file: " + path.string();
        return false;
    }
    output << secretHexOut;
    return true;
}

std::string encryptSecret(const std::string& plaintext, std::string& error)
{
    std::vector<unsigned char> key;
    std::string secretHex;
    if (!loadOrCreateLocalSecret(key, secretHex, error)) {
        return {};
    }

    std::vector<unsigned char> nonce(crypto_secretbox_NONCEBYTES);
    std::vector<unsigned char> cipher(plaintext.size() + crypto_secretbox_MACBYTES);
    randombytes_buf(nonce.data(), nonce.size());
    if (crypto_secretbox_easy(cipher.data(),
            reinterpret_cast<const unsigned char*>(plaintext.data()),
            plaintext.size(),
            nonce.data(),
            key.data())
        != 0) {
        error = "Failed to encrypt PostgreSQL secret";
        return {};
    }

    std::string packed(reinterpret_cast<const char*>(nonce.data()), nonce.size());
    packed.append(reinterpret_cast<const char*>(cipher.data()), cipher.size());
    return base64_encode_cube(packed);
}

std::string decryptSecret(const std::string& encodedCiphertext, std::string& error)
{
    std::vector<unsigned char> key;
    std::string secretHex;
    if (!loadOrCreateLocalSecret(key, secretHex, error)) {
        return {};
    }

    const auto packed = base64_decode_cube(encodedCiphertext);
    if (packed.size() < crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES) {
        error = "Stored PostgreSQL secret is invalid";
        return {};
    }

    const unsigned char* nonce = packed.data();
    const unsigned char* cipher = packed.data() + crypto_secretbox_NONCEBYTES;
    const size_t cipherSize = packed.size() - crypto_secretbox_NONCEBYTES;
    std::vector<unsigned char> plain(cipherSize - crypto_secretbox_MACBYTES);
    if (crypto_secretbox_open_easy(
            plain.data(),
            cipher,
            cipherSize,
            nonce,
            key.data())
        != 0) {
        error = "Failed to decrypt PostgreSQL secret";
        return {};
    }

    return std::string(reinterpret_cast<const char*>(plain.data()), plain.size());
}

std::string bootstrapTokenForApp(const std::string& appId, std::string& error)
{
    std::vector<unsigned char> key;
    std::string secretHex;
    if (!loadOrCreateLocalSecret(key, secretHex, error)) {
        return {};
    }
    return sha256(secretHex + ":" + appId + ":postgres-bootstrap");
}

std::string generatePassword(std::string& error)
{
    if (!ensureSodiumReady(error)) {
        return {};
    }
    std::vector<unsigned char> randomBytes(24);
    randombytes_buf(randomBytes.data(), randomBytes.size());
    return hexEncode(randomBytes.data(), randomBytes.size());
}

std::string roleNameForApp(const std::string& appId)
{
    return "pgapp_" + sha256(appId).substr(0, 24);
}

std::string actualDatabaseNameForApp(const std::string& appId, const std::string& logicalName)
{
    const auto logicalSuffix = logicalName.substr(0, std::min<size_t>(logicalName.size(), 32));
    return "pgdb_" + sha256(appId + ":" + logicalName).substr(0, 12) + "_" + logicalSuffix;
}

std::string pgIdentifier(const std::string& identifier)
{
    return "\"" + identifier + "\"";
}

int postgresPort()
{
    try {
        return std::stoi(Config::get("APPS_POSTGRES_PORT", std::to_string(kDefaultPostgresPort)));
    } catch (...) {
        return kDefaultPostgresPort;
    }
}

std::string postgresHost()
{
    return Config::get("APPS_POSTGRES_HOST", kDefaultPostgresHost);
}

std::string postgresSystemAppId()
{
    return Config::get("APPS_POSTGRES_APP_ID", kDefaultPostgresAppId);
}

std::string adminConnectionString(const std::string& dbName)
{
    std::ostringstream conn;
    conn << "host=" << postgresHost()
         << " port=" << postgresPort()
         << " dbname=" << dbName
         << " user=" << Config::get("APPS_POSTGRES_ADMIN_USER", kDefaultPostgresAdminUser);
    const auto password = Config::get("APPS_POSTGRES_ADMIN_PASSWORD", kDefaultPostgresAdminPassword);
    if (!password.empty()) {
        conn << " password=" << password;
    }
    return conn.str();
}

bool ensureAccessTable(Database* db)
{
    if (db == nullptr || !db->isOpen()) {
        return false;
    }
    if (!db->tableExists(kAccessTableName)) {
        return db->createTable(
            kAccessTableName,
            { "id", "app_id", "pg_role_name", "encrypted_password", "database_mapping_json", "bootstrap_token_hash", "provisioned_at_ms", "last_fetch_at_ms" },
            { "INTEGER PRIMARY KEY", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT" },
            { true, true, true, false, false, false, false, false });
    }
    return true;
}

std::optional<AccessRow> rowFromSelectResult(const std::vector<std::vector<std::string>>& rows)
{
    if (rows.empty() || rows.front().size() < 7) {
        return std::nullopt;
    }

    AccessRow row;
    row.appId = rows.front()[0];
    row.roleName = rows.front()[1];
    row.encryptedPassword = rows.front()[2];
    row.bootstrapTokenHash = rows.front()[4];
    row.provisionedAtMs = rows.front()[5];
    row.lastFetchAtMs = rows.front()[6];
    try {
        row.databaseMapping = rows.front()[3].empty() ? nlohmann::json::object() : nlohmann::json::parse(rows.front()[3]);
    } catch (...) {
        row.databaseMapping = nlohmann::json::object();
    }
    return row;
}

std::optional<AccessRow> getAccessRowByAppId(Database* db, const std::string& appId)
{
    if (!ensureAccessTable(db)) {
        return std::nullopt;
    }
    return rowFromSelectResult(db->selectData(
        kAccessTableName,
        { "app_id", "pg_role_name", "encrypted_password", "database_mapping_json", "bootstrap_token_hash", "provisioned_at_ms", "last_fetch_at_ms" },
        { DB_NS::Predicate { "app_id", appId } }));
}

std::optional<AccessRow> getAccessRowByBootstrapTokenHash(Database* db, const std::string& tokenHash)
{
    if (!ensureAccessTable(db)) {
        return std::nullopt;
    }
    return rowFromSelectResult(db->selectData(
        kAccessTableName,
        { "app_id", "pg_role_name", "encrypted_password", "database_mapping_json", "bootstrap_token_hash", "provisioned_at_ms", "last_fetch_at_ms" },
        { DB_NS::Predicate { "bootstrap_token_hash", tokenHash } }));
}

bool upsertAccessRow(Database* db, const AccessRow& row)
{
    if (!ensureAccessTable(db)) {
        return false;
    }

    const std::vector<std::string> columns = {
        "pg_role_name",
        "encrypted_password",
        "database_mapping_json",
        "bootstrap_token_hash",
        "provisioned_at_ms",
        "last_fetch_at_ms"
    };
    const std::vector<std::string> values = {
        row.roleName,
        row.encryptedPassword,
        row.databaseMapping.dump(),
        row.bootstrapTokenHash,
        row.provisionedAtMs,
        row.lastFetchAtMs
    };

    if (db->rowExists(kAccessTableName, { DB_NS::Predicate { "app_id", row.appId } })) {
        return db->updateData(kAccessTableName, columns, values, { DB_NS::Predicate { "app_id", row.appId } });
    }

    std::vector<std::string> insertColumns = { "app_id" };
    insertColumns.insert(insertColumns.end(), columns.begin(), columns.end());
    std::vector<std::string> insertValues = { row.appId };
    insertValues.insert(insertValues.end(), values.begin(), values.end());
    return -1 < db->insertData(kAccessTableName, insertColumns, insertValues);
}

std::vector<std::string> databaseNamesFromMapping(const nlohmann::json& mapping)
{
    std::vector<std::string> names;
    if (!mapping.is_object()) {
        return names;
    }
    for (auto it = mapping.begin(); it != mapping.end(); ++it) {
        if (it.value().is_string()) {
            names.push_back(it.value().get<std::string>());
        }
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    return names;
}

class PqxxPostgresAdminClient final : public AppPostgresAdminClient {
public:
    bool ensureProvisioned(
        const std::string& roleName,
        const std::string& password,
        const std::vector<std::string>& desiredDatabases,
        const std::vector<std::string>& removedDatabases,
        std::string& error) override
    {
        const int maxAttempts = [] {
            try {
                return std::stoi(Config::get("APPS_POSTGRES_READY_RETRIES", "20"));
            } catch (...) {
                return 20;
            }
        }();
        const int retryDelayMs = [] {
            try {
                return std::stoi(Config::get("APPS_POSTGRES_READY_RETRY_DELAY_MS", "500"));
            } catch (...) {
                return 500;
            }
        }();

        for (int attempt = 0; attempt < std::max(1, maxAttempts); ++attempt) {
            try {
                const auto adminDb = Config::get("APPS_POSTGRES_ADMIN_DB", kDefaultPostgresAdminDb);
                {
                    pqxx::connection conn(adminConnectionString(adminDb));
                    pqxx::work tx(conn);
                    const bool roleExists = !tx.exec("SELECT 1 FROM pg_roles WHERE rolname = " + tx.quote(roleName)).empty();
                    if (roleExists) {
                        execNoRows(tx, "ALTER ROLE " + pgIdentifier(roleName) + " WITH LOGIN PASSWORD " + tx.quote(password));
                    } else {
                        execNoRows(tx, "CREATE ROLE " + pgIdentifier(roleName) + " WITH LOGIN PASSWORD " + tx.quote(password));
                    }
                    const auto existingDatabases = tx.exec("SELECT datname FROM pg_database WHERE datistemplate = false");
                    for (const auto& row : existingDatabases) {
                        const std::string databaseName = row[0].c_str();
                        execNoRows(tx, "REVOKE ALL PRIVILEGES ON DATABASE " + pgIdentifier(databaseName) + " FROM " + pgIdentifier(roleName));
                        execNoRows(tx, "REVOKE CONNECT ON DATABASE " + pgIdentifier(databaseName) + " FROM " + pgIdentifier(roleName));
                    }
                    tx.commit();
                }

                for (const auto& dbName : desiredDatabases) {
                    bool databaseExists = false;
                    {
                        pqxx::connection conn(adminConnectionString(adminDb));
                        pqxx::work tx(conn);
                        databaseExists = !tx.exec("SELECT 1 FROM pg_database WHERE datname = " + tx.quote(dbName)).empty();
                        tx.commit();
                    }
                    if (!databaseExists) {
                        pqxx::connection conn(adminConnectionString(adminDb));
                        pqxx::nontransaction ntx(conn);
                        execNoRows(ntx, "CREATE DATABASE " + pgIdentifier(dbName));
                    }

                    {
                        pqxx::connection conn(adminConnectionString(adminDb));
                        pqxx::work tx(conn);
                        execNoRows(tx, "GRANT ALL PRIVILEGES ON DATABASE " + pgIdentifier(dbName) + " TO " + pgIdentifier(roleName));
                        tx.commit();
                    }
                    {
                        pqxx::connection conn(adminConnectionString(dbName));
                        pqxx::work tx(conn);
                        execNoRows(tx, "GRANT ALL ON SCHEMA public TO " + pgIdentifier(roleName));
                        execNoRows(tx, "GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO " + pgIdentifier(roleName));
                        execNoRows(tx, "GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO " + pgIdentifier(roleName));
                        execNoRows(tx, "ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL PRIVILEGES ON TABLES TO " + pgIdentifier(roleName));
                        execNoRows(tx, "ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL PRIVILEGES ON SEQUENCES TO " + pgIdentifier(roleName));
                        tx.commit();
                    }
                }

                for (const auto& dbName : removedDatabases) {
                    bool databaseExists = false;
                    {
                        pqxx::connection conn(adminConnectionString(adminDb));
                        pqxx::work tx(conn);
                        databaseExists = !tx.exec("SELECT 1 FROM pg_database WHERE datname = " + tx.quote(dbName)).empty();
                        tx.commit();
                    }
                    if (!databaseExists) {
                        continue;
                    }
                    {
                        pqxx::connection conn(adminConnectionString(adminDb));
                        pqxx::work tx(conn);
                        execNoRows(tx, "REVOKE ALL PRIVILEGES ON DATABASE " + pgIdentifier(dbName) + " FROM " + pgIdentifier(roleName));
                        tx.commit();
                    }
                    {
                        pqxx::connection conn(adminConnectionString(dbName));
                        pqxx::work tx(conn);
                        execNoRows(tx, "REVOKE ALL ON SCHEMA public FROM " + pgIdentifier(roleName));
                        execNoRows(tx, "REVOKE ALL PRIVILEGES ON ALL TABLES IN SCHEMA public FROM " + pgIdentifier(roleName));
                        execNoRows(tx, "REVOKE ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public FROM " + pgIdentifier(roleName));
                        tx.commit();
                    }
                }

                return true;
            } catch (const std::exception& e) {
                error = e.what();
                if (attempt + 1 < std::max(1, maxAttempts)) {
                    genericSleep(retryDelayMs);
                }
            }
        }

        return false;
    }
};

std::shared_ptr<AppPostgresAdminClient> adminClient()
{
    std::scoped_lock lock(gAdminClientMutex);
    if (gAdminClientForTests != nullptr) {
        return gAdminClientForTests;
    }
    static auto defaultClient = std::make_shared<PqxxPostgresAdminClient>();
    return defaultClient;
}
} // namespace

bool AppPostgresAccess::parseRequestedDatabases(const nlohmann::json& manifest, std::vector<std::string>& logicalDatabases, std::string& error)
{
    logicalDatabases.clear();
    if (!manifest.is_object() || !manifest.contains("permissions") || !manifest["permissions"].is_object()) {
        return true;
    }

    const auto& permissions = manifest["permissions"];
    if (!permissions.contains("postgresql")) {
        return true;
    }
    if (!permissions["postgresql"].is_object()) {
        error = "permissions.postgresql must be an object";
        return false;
    }
    const auto& postgres = permissions["postgresql"];
    if (!postgres.contains("databases")) {
        error = "permissions.postgresql.databases is required when permissions.postgresql is present";
        return false;
    }
    if (!postgres["databases"].is_array()) {
        error = "permissions.postgresql.databases must be an array";
        return false;
    }

    std::set<std::string> seen;
    for (const auto& entry : postgres["databases"]) {
        if (!entry.is_string()) {
            error = "permissions.postgresql.databases entries must be strings";
            return false;
        }
        const auto logicalName = entry.get<std::string>();
        if (!std::regex_match(logicalName, kLogicalDatabasePattern)) {
            error = "permissions.postgresql.databases entries must match ^[a-z][a-z0-9_]*$";
            return false;
        }
        if (seen.insert(logicalName).second) {
            logicalDatabases.push_back(logicalName);
        }
    }

    return true;
}

bool AppPostgresAccess::provisionForApp(
    const std::string& appId,
    const nlohmann::json& manifest,
    const std::function<bool(const std::string&, std::string&)>& ensureSystemAppRunning,
    std::string& error)
{
    std::vector<std::string> logicalDatabases;
    if (!parseRequestedDatabases(manifest, logicalDatabases, error)) {
        return false;
    }

    auto* db = CubeDB::getDBManager()->getDatabase(kAppsDbName);
    if (db == nullptr || !db->isOpen()) {
        error = "Apps DB is not available";
        return false;
    }
    if (!ensureAccessTable(db)) {
        error = db->getLastError();
        return false;
    }

    const auto existingRow = getAccessRowByAppId(db, appId);
    if (logicalDatabases.empty()) {
        if (!existingRow.has_value()) {
            return true;
        }

        const auto postgresAppId = postgresSystemAppId();
        if (!postgresAppId.empty() && postgresAppId != appId) {
            std::string startupError;
            if (!ensureSystemAppRunning(postgresAppId, startupError)) {
                error = startupError.empty() ? "PostgreSQL system app is not running" : startupError;
                return false;
            }
        }

        std::string password = decryptSecret(existingRow->encryptedPassword, error);
        if (password.empty()) {
            return false;
        }
        const auto previousDatabases = databaseNamesFromMapping(existingRow->databaseMapping);
        if (!adminClient()->ensureProvisioned(existingRow->roleName, password, {}, previousDatabases, error)) {
            return false;
        }
        if (!db->deleteData(kAccessTableName, { DB_NS::Predicate { "app_id", appId } })) {
            error = db->getLastError();
            return false;
        }
        return true;
    }

    const auto postgresAppId = postgresSystemAppId();
    if (!postgresAppId.empty() && postgresAppId != appId) {
        std::string startupError;
        if (!ensureSystemAppRunning(postgresAppId, startupError)) {
            error = startupError.empty() ? "PostgreSQL system app is not running" : startupError;
            return false;
        }
    }

    std::string password;
    if (existingRow.has_value()) {
        password = decryptSecret(existingRow->encryptedPassword, error);
        if (password.empty()) {
            return false;
        }
    } else {
        password = generatePassword(error);
        if (password.empty()) {
            return false;
        }
    }

    nlohmann::json mapping = nlohmann::json::object();
    for (const auto& logicalName : logicalDatabases) {
        mapping[logicalName] = actualDatabaseNameForApp(appId, logicalName);
    }
    const auto desiredDatabases = databaseNamesFromMapping(mapping);
    const auto previousDatabases = existingRow.has_value() ? databaseNamesFromMapping(existingRow->databaseMapping) : std::vector<std::string> {};
    std::vector<std::string> removedDatabases;
    std::set_difference(
        previousDatabases.begin(),
        previousDatabases.end(),
        desiredDatabases.begin(),
        desiredDatabases.end(),
        std::back_inserter(removedDatabases));

    if (!adminClient()->ensureProvisioned(roleNameForApp(appId), password, desiredDatabases, removedDatabases, error)) {
        return false;
    }

    const auto encryptedPassword = encryptSecret(password, error);
    if (encryptedPassword.empty()) {
        return false;
    }
    const auto bootstrapToken = bootstrapTokenForApp(appId, error);
    if (bootstrapToken.empty()) {
        return false;
    }

    AccessRow row;
    row.appId = appId;
    row.roleName = roleNameForApp(appId);
    row.encryptedPassword = encryptedPassword;
    row.databaseMapping = mapping;
    row.bootstrapTokenHash = sha256(bootstrapToken);
    row.provisionedAtMs = existingRow.has_value() && !existingRow->provisionedAtMs.empty() ? existingRow->provisionedAtMs : std::to_string(epochMsNow());
    row.lastFetchAtMs = existingRow.has_value() ? existingRow->lastFetchAtMs : "";

    if (!upsertAccessRow(db, row)) {
        error = db->getLastError();
        return false;
    }

    return true;
}

bool AppPostgresAccess::appendLaunchEnvironment(
    const std::string& appId,
    const std::vector<std::string>& logicalDatabases,
    nlohmann::json& environmentSet,
    std::string& error)
{
    if (logicalDatabases.empty()) {
        return true;
    }

    const auto bootstrapToken = bootstrapTokenForApp(appId, error);
    if (bootstrapToken.empty()) {
        return false;
    }

    environmentSet[bootstrapEnvVarName()] = bootstrapToken;
    environmentSet[endpointEnvVarName()] = kCredentialsEndpointPath;
    return true;
}

void AppPostgresAccess::setAdminClientForTests(std::shared_ptr<AppPostgresAdminClient> adminClientOverride)
{
    std::scoped_lock lock(gAdminClientMutex);
    gAdminClientForTests = std::move(adminClientOverride);
}

void AppPostgresAccess::clearAdminClientForTests()
{
    std::scoped_lock lock(gAdminClientMutex);
    gAdminClientForTests.reset();
}

std::string AppPostgresAccess::bootstrapHeaderName()
{
    return kBootstrapHeaderName;
}

std::string AppPostgresAccess::bootstrapEnvVarName()
{
    return kBootstrapEnvVarName;
}

std::string AppPostgresAccess::endpointEnvVarName()
{
    return kEndpointEnvVarName;
}

HttpEndPointData_t AppPostgresAccess::getHttpEndpointData()
{
    HttpEndPointData_t data;
    data.push_back({
        PRIVATE_ENDPOINT | POST_ENDPOINT | IPC_ONLY_ENDPOINT,
        [](const httplib::Request& req, httplib::Response& res) {
            nlohmann::json response;
            const auto token = req.get_header_value(kBootstrapHeaderName);
            if (token.empty()) {
                response["success"] = false;
                response["message"] = "Missing bootstrap token";
                res.status = httplib::StatusCode::Forbidden_403;
                res.set_content(response.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }

            auto* db = CubeDB::getDBManager()->getDatabase(kAppsDbName);
            if (db == nullptr || !db->isOpen() || !ensureAccessTable(db)) {
                response["success"] = false;
                response["message"] = "Apps DB unavailable";
                res.status = httplib::StatusCode::InternalServerError_500;
                res.set_content(response.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }

            const auto row = getAccessRowByBootstrapTokenHash(db, sha256(token));
            if (!row.has_value()) {
                response["success"] = false;
                response["message"] = "Bootstrap token is invalid";
                res.status = httplib::StatusCode::Forbidden_403;
                res.set_content(response.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }

            std::string error;
            const auto password = decryptSecret(row->encryptedPassword, error);
            if (password.empty()) {
                response["success"] = false;
                response["message"] = error.empty() ? "Failed to decrypt credentials" : error;
                res.status = httplib::StatusCode::InternalServerError_500;
                res.set_content(response.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            }

            db->updateData(
                kAccessTableName,
                { "last_fetch_at_ms" },
                { std::to_string(epochMsNow()) },
                { DB_NS::Predicate { "app_id", row->appId } });

            response["success"] = true;
            response["host"] = postgresHost();
            response["port"] = postgresPort();
            response["username"] = row->roleName;
            response["password"] = password;
            response["databases"] = row->databaseMapping;
            res.status = httplib::StatusCode::OK_200;
            res.set_content(response.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "fetchCredentials",
        nlohmann::json({
            { "type", "object" },
            { "properties", nlohmann::json::object() }
        }),
        "Fetch the provisioned PostgreSQL credentials for the calling app over IPC."
    });
    return data;
}
