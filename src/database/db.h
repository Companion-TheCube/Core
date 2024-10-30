#ifndef DB_H
#define DB_H

#include <SQLiteCpp/SQLiteCpp.h>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <logger.h>
#include <string>
#include <utils.h>
#include <vector>

namespace DB_NS {
typedef struct Table_T {
    const std::string name;
    const std::vector<std::string> columnNames;
    const std::vector<std::string> columnTypes;
    const std::vector<bool> columnUnique;
} Table_T;

typedef struct Database_T {
    const std::string path;
    const std::string name;
    const std::vector<Table_T> tables;
} DB_T;

namespace Roles {
    const std::string ROLE_CUBE = "CUBE";
    const std::string ROLE_MINI_CUBE = "MINI_CUBE";
    const std::string ROLE_NATIVE_APP = "NATIVE_APP";
    const std::string ROLE_DOCKER_APP = "DOCKER_APP";
    const std::string ROLE_WEB_APP = "WEB_APP";
    const std::string ROLE_MOBILE_APP = "MOBILE_APP";
    const std::string ROLE_UNKNOWN = "UNKNOWN";
};
namespace TableNames {
    const std::string CLIENTS = "clients";
    const std::string APPS = "apps";
    const std::string CLIENT_BLOBS = "client_blobs";
    const std::string APP_BLOBS = "app_blobs";
    const std::string ACCOUNTS = "accounts";
    const std::string NOTIFICATIONS = "notifications";
}
// clang-format off
const std::vector<DB_NS::Database_T> dbDefs = {
    { 
        "data/auth.db", "auth", 
        { 
            { 
                DB_NS::TableNames::CLIENTS, 
                { "id", "initial_code", "auth_code", "client_id", "role" }, 
                { "INTEGER PRIMARY KEY", "TEXT", "TEXT", "TEXT", "INTEGER" }, 
                { true, false, false, true, false } 
            }, 
            { 
                DB_NS::TableNames::APPS, 
                { "id", "auth_code", "public_key", "private_key", "app_id", "role" }, 
                { "INTEGER PRIMARY KEY", "TEXT", "TEXT", "TEXT", "TEXT", "INTEGER" }, 
                { true, false, false, false, true, false } 
            } 
        } 
    },
    { 
        "data/blobs.db", "blobs", 
        { 
            { 
                DB_NS::TableNames::CLIENT_BLOBS, 
                { "id", "blob", "blob_size", "owner_client_id" }, 
                { "INTEGER PRIMARY KEY", "BLOB", "TEXT", "TEXT" }, 
                { true, false, false, false } 
            }, 
            { 
                DB_NS::TableNames::APP_BLOBS, 
                { "id", "blob", "blob_size", "owner_app_id" }, 
                { "INTEGER PRIMARY KEY", "BLOB", "TEXT", "TEXT" }, 
                { true, false, false, false } 
            } 
        } 
    },
    { 
        "data/apps.db", "apps", 
        { 
            { 
                DB_NS::TableNames::APPS, 
                { "id", "app_id", "app_name", "enabled", "role", "exec_path", "exec_args", "app_source", "update_path", "update_last_check", "update_last_update", "update_last_fail", "update_last_fail_reason" }, 
                { "INTEGER PRIMARY KEY", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT" }, 
                { true, true, false, false, false, false, false, false, false, false, false, false, false } 
            } 
        } 
    },
    { 
        "data/accounts.db", "accounts", 
        { 
            { 
                DB_NS::TableNames::ACCOUNTS, 
                { "id", "username", "password", "service" }, 
                { "INTEGER PRIMARY KEY", "TEXT", "TEXT", "TEXT" }, 
                { true, false, false, false } 
            } 
        } 
    },
    { 
        "data/notifications.db", "notifications", 
        { 
            { 
                DB_NS::TableNames::NOTIFICATIONS, 
                { "id", "title", "message", "time", "source", "read" }, 
                { "INTEGER PRIMARY KEY", "TEXT", "TEXT", "TEXT", "TEXT", "INTEGER" }, 
                { true, false, false, false, false, false } 
            } 
        } 
    }
};
// clang-format on
} // namespace DB_NS

class Database {
    std::string dbPath;
    SQLite::Database* db;
    std::string lastError;
    bool openFlag;
    std::string dbName;
    std::map<std::string, bool> uniqueColumns;

public:
    Database(const std::string& dbPath);
    ~Database();
    bool createTable(const std::string& tableName, std::vector<std::string> columnNames, std::vector<std::string> columnTypes, std::vector<bool> uniqueColumns);
    long insertData(const std::string& tableName, std::vector<std::string> columnNames, std::vector<std::string> columnValues);
    bool updateData(const std::string& tableName, std::vector<std::string> columnNames, std::vector<std::string> columnValues, const std::string& whereClause);
    void setUniqueColumns(std::vector<std::string> columnNames, std::vector<bool> uniqueColumns);
    bool deleteData(const std::string& tableName, const std::string& whereClause);
    std::vector<std::vector<std::string>> selectData(const std::string& tableName, std::vector<std::string> columnNames, const std::string& whereClause);
    std::vector<std::vector<std::string>> selectData(const std::string& tableName, std::vector<std::string> columnNames);
    std::vector<std::vector<std::string>> selectData(const std::string& tableName);
    char* selectBlob(const std::string& tableName, const std::string& columnName, const std::string& whereClause, int& size);
    std::string selectBlobString(const std::string& tableName, const std::string& columnName, const std::string& whereClause);
    bool tableExists(const std::string& tableName);
    bool columnExists(const std::string& tableName, const std::string& columnName);
    bool rowExists(const std::string& tableName, const std::string& whereClause);
    bool open();
    bool close();
    bool isOpen();
    std::string getDBPath();
    std::string getDBName();
    std::string getLastError();
    bool createDB(const std::string& dbPath);
};

class CubeDatabaseManager {
    std::vector<Database*> databases;
    TaskQueue dbQueue;
    std::jthread dbThread;
    std::stop_token dbStopToken;
    void dbWorker();
    std::mutex dbMutex;
    bool isReady = false;

public:
    CubeDatabaseManager();
    ~CubeDatabaseManager();
    void addDatabase(const std::string& dbPath);
    Database* getDatabase(const std::string& dbName);
    bool removeDatabase(const std::string& dbName);
    std::vector<Database*> getDatabases();
    void closeAll();
    void openAll();
    void closeDatabase(const std::string& dbName);
    bool openDatabase(const std::string& dbName);
    void addDbTask(std::function<void()> task);
    bool isDatabaseManagerReady();
};

class BlobsManager {
    std::shared_ptr<CubeDatabaseManager> dbManager;
    std::mutex blobsMutex;
    bool isReady = false;

public:
    BlobsManager(std::shared_ptr<CubeDatabaseManager> dbManager, const std::string& dbPath);
    ~BlobsManager();
    int addBlob(const std::string& tableName, const std::string& blob, const std::string& ownerID);
    int addBlob(const std::string& tableName, char* blob, int size, const std::string& ownerID);
    bool removeBlob(const std::string& tableName, const std::string& ownerID, int id);
    std::string getBlobString(const std::string& tableName, const std::string& ownerID, int id);
    char* getBlobChars(const std::string& tableName, const std::string& ownerID, int id, int& size);
    bool updateBlob(const std::string& tableName, const std::string& blob, const std::string& ownerID, int id);
    bool updateBlob(const std::string& tableName, char* blob, int size, const std::string& ownerID, int id);
    bool isBlobsManagerReady();
};

// TODO: make sure all the data is sanitized before being inserted into the database.
bool sanitizeString(std::string& str);

#endif // DB_H
