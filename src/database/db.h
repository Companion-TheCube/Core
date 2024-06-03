#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <logger.h>
#include <exception>



namespace DB_NS{
    typedef struct Table_T{
        std::string name;
        std::vector<std::string> columnNames;
        std::vector<std::string> columnTypes;
    } Table_T;

    typedef struct Database_T{
        std::string path;
        std::string name;
        std::vector<Table_T> tables;
    } DB_T;

    enum Roles{
        ROLE_CUBE,
        ROLE_MINI_CUBE,
        ROLE_APP,
        ROLE_DOCKER_APP,
        ROLE_MOBILE_APP,
        ROLE_WEB_APP
    };
    namespace TableNames{
        const std::string CLIENTS = "clients";
        const std::string APPS = "apps";
        const std::string CLIENT_BLOBS = "client_blobs";
        const std::string APP_BLOBS = "app_blobs";
    }
}
class Database {
    CubeLog *logger;
    std::string dbPath;
    SQLite::Database *db;
    std::string lastError;
    bool openFlag;
    std::string dbName;
public:
    Database(CubeLog *logger, std::string dbPath);
    ~Database();
    bool createTable(std::string tableName, std::vector<std::string> columnNames, std::vector<std::string> columnTypes);
    bool insertData(std::string tableName, std::vector<std::string> columnNames, std::vector<std::string> columnValues);
    bool updateData(std::string tableName, std::vector<std::string> columnNames, std::vector<std::string> columnValues, std::string whereClause);
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
    CubeLog *logger;
    std::vector<Database*> databases;
    std::vector<DB_NS::Database_T> dbDefs= {
        {"data/auth.db", "auth", {
            {DB_NS::TableNames::CLIENTS, {"id", "initial_code", "auth_code", "client_id", "role"}, {"INTEGER", "TEXT", "TEXT", "TEXT", "INTEGER"}},
            {DB_NS::TableNames::APPS, {"id", "sequence_key", "public_key", "private_key", "app_id", "role"}, {"INTEGER", "TEXT", "TEXT", "TEXT", "TEXT", "INTEGER"}}
        }},
        {"data/blobs.db", "blobs", {
            {DB_NS::TableNames::CLIENT_BLOBS, {"id", "blob", "owner_client_id"}, {"INTEGER", "BLOB", "TEXT"}},
            {DB_NS::TableNames::APP_BLOBS, {"id", "blob", "owner_app_id"}, {"INTEGER", "BLOB", "TEXT"}}
        }}
    };
public:
    CubeDatabaseManager(CubeLog *logger);
    ~CubeDatabaseManager();
    void addDatabase(std::string dbPath);
    Database* getDatabase(std::string dbName);
    bool removeDatabase(std::string dbName);
    std::vector<Database*> getDatabases();
    void closeAll();
    void openAll();
    void closeDatabase(std::string dbName);
    bool openDatabase(std::string dbName);

};

class BlobsManager{
    CubeLog *logger;
    CubeDatabaseManager *dbManager;
public:
    BlobsManager(CubeLog *logger, CubeDatabaseManager *dbManager, std::string dbPath);
    ~BlobsManager();
    bool addBlob(std::string tableName, std::string blob, std::string ownerID);
    bool addBlob(std::string tableName, char* blob, int size, std::string ownerID);
    bool removeBlob(std::string tableName, std::string ownerID, int id);
    std::string getBlobString(std::string tableName, std::string ownerID, int id);
    char* getBlobChars(std::string tableName, std::string ownerID, int id, int &size);
    bool updateBlob(std::string tableName, std::string blob, std::string ownerID, int id);
    bool updateBlob(std::string tableName, char* blob, int size, std::string ownerID, int id);
};