#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <logger.h>
#include <exception>
#include <functional>
#include <utils.h>

namespace DB_NS{
    typedef struct Table_T{
        std::string name;
        std::vector<std::string> columnNames;
        std::vector<std::string> columnTypes;
        std::vector<bool> columnUnique;
    } Table_T;

    typedef struct Database_T{
        std::string path;
        std::string name;
        std::vector<Table_T> tables;
    } DB_T;

    namespace Roles{
        const std::string ROLE_CUBE = "CUBE";
        const std::string ROLE_MINI_CUBE = "MINI_CUBE";
        const std::string ROLE_NATIVE_APP = "NATIVE_APP";
        const std::string ROLE_DOCKER_APP = "DOCKER_APP";
        const std::string ROLE_WEB_APP = "WEB_APP";
        const std::string ROLE_MOBILE_APP = "MOBILE_APP";
        const std::string ROLE_UNKNOWN = "UNKNOWN";
    };
    namespace TableNames{
        const std::string CLIENTS = "clients";
        const std::string APPS = "apps";
        const std::string CLIENT_BLOBS = "client_blobs";
        const std::string APP_BLOBS = "app_blobs";
    }
}
class Database {
    std::string dbPath;
    SQLite::Database *db;
    std::string lastError;
    bool openFlag;
    std::string dbName;
    std::map<std::string,bool> uniqueColumns;
public:
    Database(std::string dbPath);
    ~Database();
    bool createTable(std::string tableName, std::vector<std::string> columnNames, std::vector<std::string> columnTypes, std::vector<bool> uniqueColumns);
    long insertData(std::string tableName, std::vector<std::string> columnNames, std::vector<std::string> columnValues);
    bool updateData(std::string tableName, std::vector<std::string> columnNames, std::vector<std::string> columnValues, std::string whereClause);
    void setUniqueColumns(std::vector<std::string> columnNames, std::vector<bool> uniqueColumns);
    bool deleteData(std::string tableName, std::string whereClause);
    std::vector<std::vector<std::string>> selectData(std::string tableName, std::vector<std::string> columnNames, std::string whereClause);
    std::vector<std::vector<std::string>> selectData(std::string tableName, std::vector<std::string> columnNames);
    std::vector<std::vector<std::string>> selectData(std::string tableName);
    char* selectBlob(std::string tableName, std::string columnName, std::string whereClause, int &size);
    std::string selectBlobString(std::string tableName, std::string columnName, std::string whereClause);
    bool tableExists(std::string tableName);
    bool columnExists(std::string tableName, std::string columnName);
    bool rowExists(std::string tableName, std::string whereClause);
    bool open();
    bool close();
    bool isOpen();
    std::string getDBPath();
    std::string getDBName();
    std::string getLastError();
    bool createDB(std::string dbPath);
};

class CubeDatabaseManager{
    std::vector<Database*> databases;
    std::vector<DB_NS::Database_T> dbDefs= {
        {"data/auth.db", "auth", {
            {DB_NS::TableNames::CLIENTS, 
                {"id", "initial_code", "auth_code", "client_id", "role"}, 
                {"INTEGER PRIMARY KEY", "TEXT", "TEXT", "TEXT", "INTEGER"},
                {true, false, false, true, false}
            },
            {DB_NS::TableNames::APPS, 
                {"id", "auth_code", "public_key", "private_key", "app_id", "role"}, 
                {"INTEGER PRIMARY KEY", "TEXT", "TEXT", "TEXT", "TEXT", "INTEGER"},
                {true, false, false, false, true, false}
            }
        }},
        {"data/blobs.db", "blobs", {
            {DB_NS::TableNames::CLIENT_BLOBS, 
                {"id", "blob", "blob_size", "owner_client_id"}, 
                {"INTEGER PRIMARY KEY", "BLOB", "TEXT", "TEXT"},
                {true, false, false, false}
            },
            {DB_NS::TableNames::APP_BLOBS, 
                {"id", "blob", "blob_size", "owner_app_id"}, 
                {"INTEGER PRIMARY KEY", "BLOB", "TEXT", "TEXT"},
                {true, false, false, false}
            }
        }},
        {"data/apps.db", "apps", {
            {DB_NS::TableNames::APPS, 
                {"id", "app_id", "app_name", "role", "exec_path", "exec_args", "app_source", "update_path", "update_last_check", "update_last_update", "update_last_fail", "update_last_fail_reason"}, 
                {"INTEGER PRIMARY KEY", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT", "TEXT"},
                {true, true, false, false, false, false, false, false, false, false, false, false}
            }
        }}
    };
    TaskQueue dbQueue;
    std::jthread dbThread;
    std::stop_token dbStopToken;
    void dbWorker();
    std::mutex dbMutex;
    bool isReady = false;
public:
    CubeDatabaseManager();
    ~CubeDatabaseManager();
    void addDatabase(std::string dbPath);
    Database* getDatabase(std::string dbName);
    bool removeDatabase(std::string dbName);
    std::vector<Database*> getDatabases();
    void closeAll();
    void openAll();
    void closeDatabase(std::string dbName);
    bool openDatabase(std::string dbName);
    void addDbTask(std::function<void()> task);
    bool isDatabaseManagerReady();
};

class BlobsManager{
    std::shared_ptr<CubeDatabaseManager> dbManager;
    std::mutex blobsMutex;
    bool isReady = false;
public:
    BlobsManager(std::shared_ptr<CubeDatabaseManager> dbManager, std::string dbPath);
    ~BlobsManager();
    bool addBlob(std::string tableName, std::string blob, std::string ownerID);
    bool addBlob(std::string tableName, char* blob, int size, std::string ownerID);
    bool removeBlob(std::string tableName, std::string ownerID, int id);
    std::string getBlobString(std::string tableName, std::string ownerID, int id);
    char* getBlobChars(std::string tableName, std::string ownerID, int id, int &size);
    bool updateBlob(std::string tableName, std::string blob, std::string ownerID, int id);
    bool updateBlob(std::string tableName, char* blob, int size, std::string ownerID, int id);
    bool isBlobsManagerReady();
};

