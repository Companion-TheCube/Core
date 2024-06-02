#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <logger.h>
#include <exception>

typedef struct Table_T{
    std::string name;
    std::vector<std::string> columnNames;
    std::vector<std::string> columnTypes;
} Table_T;

typedef struct DB_T{
    std::string path;
    std::string name;
    std::vector<Table_T> tables;
} DB_T;



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
    std::vector<DB_T> dbDefs= {
        {"data/auth.db", "auth", {
            {"clients", {"id", "initial_code", "auth_code", "key", "role"}, {"INTEGER", "TEXT", "TEXT", "TEXT", "INTEGER"}},
            {"role_permissions", {"role_id", "permission_id"}, {"INTEGER", "INTEGER"}}
        }},
        {"data/blobs.db", "blobs", {
            {"blobs", {"id", "blob", "owner_key"}, {"INTEGER", "BLOB", "TEXT"}}
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