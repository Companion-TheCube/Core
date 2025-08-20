/*
██████╗ ██████╗     ██████╗██████╗ ██████╗ 
██╔══██╗██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
██║  ██║██████╔╝   ██║     ██████╔╝██████╔╝
██║  ██║██╔══██╗   ██║     ██╔═══╝ ██╔═══╝ 
██████╔╝██████╔╝██╗╚██████╗██║     ██║     
╚═════╝ ╚═════╝ ╚═╝ ╚═════╝╚═╝     ╚═╝
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

// TODO: this file needs a line by line evaluation

#include "db.h"

/**
 * @brief Determine if the table name is one of the blobs table names
 *
 * @param tableName
 * @return true
 * @return false
 */
inline bool isInBlobsTableNames(const std::string& tableName)
{
    return tableName == DB_NS::TableNames::APP_BLOBS || tableName == DB_NS::TableNames::CLIENT_BLOBS;
}

/**
 * @brief Construct a new Database object
 *
 * @param dbPath
 */
Database::Database(const std::string& dbPath)
{
    this->dbPath = dbPath;
    this->db = nullptr;
    this->lastError = "";
    this->openFlag = false;
    this->dbName = std::filesystem::path(dbPath).filename().string().substr(0, std::filesystem::path(dbPath).filename().string().find_last_of("."));
    if (!std::filesystem::exists(dbPath)) {
        if (!this->createDB(dbPath)) {
            CubeLog::error("Failed to create database: " + dbPath);
            CubeLog::error("Last error: " + this->lastError);
        }
    }
    CubeLog::info("Database initialized: " + dbPath);
}

/**
 * @brief Destroy the Database object
 *
 */
Database::~Database()
{
    // CubeLog::info("Database closing: " + this->dbPath);
    this->close();
}

/**
 * @brief Create a table in the database
 *
 * @param tableName
 * @param columnNames Size of columnNames and columnTypes must be equal
 * @param columnTypes
 * @return true
 * @return false
 */
bool Database::createTable(const std::string& tableName, std::vector<std::string> columnNames, std::vector<std::string> columnTypes, std::vector<bool> uniqueColumns)
{
    if (columnNames.size() != columnTypes.size() || columnTypes.size() != uniqueColumns.size()) {
        this->lastError = "Column names, column types, and unique columns must have the same size";
        return false;
    }
    if (!this->isOpen()) {
        this->lastError = "Database is not open";
        return false;
    }
    CubeLog::info("Creating table: " + tableName);
    // Create a map of column names and unique columns
    for (size_t col = 0; col < columnNames.size(); col++) {
        this->uniqueColumns[columnNames.at(col)] = uniqueColumns[col];
    }
    std::string query = "CREATE TABLE IF NOT EXISTS " + tableName + " (";
    for (size_t i = 0; i < columnNames.size(); i++) {
        query += columnNames[i] + " " + columnTypes[i];
        if (i < columnNames.size() - 1) {
            query += ", ";
        }
    }
    query += ");";
    try {
        SQLite::Statement stmt(*this->db, query);
        stmt.executeStep();
        CubeLog::info("Table created: " + tableName);
        return true;
    } catch (std::exception& e) {
        this->lastError = e.what();
        return false;
    }
}

/**
 * @brief Insert data into a table
 *
 * @param tableName Table name std::string
 * @param tableEntries Vector of Table_Entry objects. Each object contains the column name, value, and (optionally) type. Accepts vector of { "columnName", "columnValue" } or vector of { "columnName", "columnValue", "columnType" }. Insertion will fail if column type does not match the existing column type.
 * @return long -1 on error, rowid on success
 */
long Database::insertData(const std::string& tableName, std::vector<DB_NS::Table_Entry> tableEntries)
{
    std::vector<std::string> columnNames;
    std::vector<std::string> columnValues;
    for (size_t i = 0; i < tableEntries.size(); i++) {
        columnNames.push_back(tableEntries[i].columnName);
        columnValues.push_back(tableEntries[i].columnValue);
    }
    return this->insertData(tableName, columnNames, columnValues);
}

/**
 * @brief Insert blob data into a table
 *
 * @param tableName
 * @param columnNames Size of columnNames and columnValues must be equal
 * @param columnValues
 * @return long -1 on error, rowid on success
 */
long Database::insertData(const std::string& tableName, std::vector<std::string> columnNames, std::vector<std::string> columnValues)
{
    if (!this->isOpen()) {
        this->lastError = "Database is not open";
        return -1;
    }
    if (columnNames.size() != columnValues.size()) {
        this->lastError = "Sizes of column names and values do not match";
        return -1;
    }
    // Check if the column is unique
    for (size_t col = 0; col < columnNames.size(); col++) {
        if (columnNames.at(col) == "blob") {
            // skip the blob column
            continue;
        }
        CubeLog::debug("Checking column: " + columnNames.at(col));
        if (this->uniqueColumns.find(columnNames.at(col)) != this->uniqueColumns.end() && this->uniqueColumns[columnNames.at(col)]) {
            CubeLog::debug("Column is marked as unique. Checking if the value already exists.");
            // check if the columnValue is empty, indicating that we should add this data with the next available value
            if (columnValues.at(col) == "") {
                CubeLog::debug("Column value is empty. Getting next available value.");
                int id = 1;
                while (this->rowExists(tableName, columnNames[col] + " = " + std::to_string(id))) {
                    id++;
                    if (id > 1000000) {
                        this->lastError = "Unable to get unique value. Too many rows in the table.";
                        return -1;
                    }
                }
                CubeLog::debug("Next available value: " + std::to_string(id));
                columnValues[col] = std::to_string(id);
            } else {
                CubeLog::debug("Column value is not empty. Checking if the value already exists.");
                if (this->rowExists(tableName, columnNames.at(col) + " = '" + columnValues.at(col) + "'")) {
                    this->lastError = "Unique column value already exists";
                    return -1;
                }
            }
        }
    }

    std::string query = "INSERT INTO " + tableName + " (";
    std::vector<int> blobColumns;
    for (size_t i = 0; i < columnNames.size(); i++) {
        if (columnNames.at(i) == "blob") {
            blobColumns.push_back(i);
        }
        query += columnNames.at(i);
        if (i < columnNames.size() - 1) {
            query += ", ";
        }
    }
    query += ") VALUES (";
    for (size_t i = 0; i < columnValues.size(); i++) {
        query += "?";
        if (i < columnValues.size() - 1) {
            query += ", ";
        }
    }
    query += ");";
    try {
        SQLite::Statement stmt(*this->db, query);
        for (size_t i = 0; i < columnNames.size(); i++) {
            if (std::find(blobColumns.begin(), blobColumns.end(), i) != blobColumns.end()) {
                // const char* blob = columnValues.at(i).c_str();
                stmt.bind(i + 1, columnValues.at(i).c_str(), columnValues.at(i).size());
            } else {
                stmt.bind(i + 1, columnValues.at(i));
            }
        }
        stmt.exec();
        return this->db->getLastInsertRowid();
    } catch (std::exception& e) {
        this->lastError = e.what();
        return -1;
    }
}

/**
 * @brief Update data in a table
 *
 * @param tableName
 * @param columnNames Size of columnNames and columnValues must be equal
 * @param columnValues
 * @param whereClause
 * @return true
 * @return false
 */
bool Database::updateData(const std::string& tableName, std::vector<std::string> columnNames, std::vector<std::string> columnValues, const std::string& whereClause)
{
    if (!this->isOpen()) {
        this->lastError = "Database is not open";
        return false;
    }
    if (columnNames.size() != columnValues.size()) {
        this->lastError = "Column names and values do not match";
        return false;
    }
    std::string query = "UPDATE " + tableName + " SET ";
    for (size_t i = 0; i < columnNames.size(); i++) {
        query += columnNames[i] + " = '" + columnValues[i] + "'";
        if (i < columnNames.size() - 1) {
            query += ", ";
        }
    }
    query += " WHERE " + whereClause + ";";
    try {
        SQLite::Statement stmt(*this->db, query);
        stmt.executeStep();
        return true;
    } catch (std::exception& e) {
        this->lastError = e.what();
        return false;
    }
}

/**
 * @brief Set the unique columns for a table
 *
 * @param tableName
 * @param uniqueColumns
 */
void Database::setUniqueColumns(std::vector<std::string> columnNames, std::vector<bool> uniqueColumns)
{
    if (columnNames.size() != uniqueColumns.size()) {
        this->lastError = "Column names and unique columns do not match";
        return;
    }
    for (size_t i = 0; i < uniqueColumns.size(); i++) {
        this->uniqueColumns[columnNames.at(i)] = uniqueColumns[i];
    }
}

/**
 * @brief Delete data from a table
 *
 * @param tableName
 * @param whereClause
 * @return true
 * @return false
 */
bool Database::deleteData(const std::string& tableName, const std::string& whereClause)
{
    if (!this->isOpen()) {
        this->lastError = "Database is not open";
        return false;
    }
    std::string query = "DELETE FROM " + tableName + " WHERE " + whereClause + ";";
    try {
        SQLite::Statement stmt(*this->db, query);
        stmt.executeStep();
        return true;
    } catch (std::exception& e) {
        this->lastError = e.what();
        return false;
    }
}

/**
 * @brief Select data from a table. Selects specific columns from the table and specific rows in the table.
 *
 * @param tableName
 * @param columnNames
 * @param whereClause
 * @return std::vector<std::vector<std::string>>
 */
std::vector<std::vector<std::string>> Database::selectData(const std::string& tableName, std::vector<std::string> columnNames, const std::string& whereClause)
{
    if (!this->isOpen()) {
        this->lastError = "Database is not open";
        return {};
    }
    std::string query = "SELECT ";
    for (size_t i = 0; i < columnNames.size(); i++) {
        query += columnNames[i];
        if (i < columnNames.size() - 1) {
            query += ", ";
        }
    }
    query += " FROM " + tableName + " WHERE " + whereClause + ";";
    try {
        SQLite::Statement stmt(*this->db, query);
        std::vector<std::vector<std::string>> results;
        while (stmt.executeStep()) {
            std::vector<std::string> row;
            for (size_t i = 0; i < columnNames.size(); i++) {
                row.push_back(stmt.getColumn(i).getText());
            }
            results.push_back(row);
        }
        return results;
    } catch (std::exception& e) {
        this->lastError = e.what();
        return {};
    }
}

/**
 * @brief Select data from a table. Selects specific columns from the table and all rows in the table.
 *
 * @param tableName
 * @param columnNames
 * @return std::vector<std::vector<std::string>>
 */
std::vector<std::vector<std::string>> Database::selectData(const std::string& tableName, std::vector<std::string> columnNames)
{
    if (!this->isOpen()) {
        this->lastError = "Database is not open";
        return {};
    }
    std::string query = "SELECT ";
    for (size_t i = 0; i < columnNames.size(); i++) {
        query += columnNames[i];
        if (i < columnNames.size() - 1) {
            query += ", ";
        }
    }
    query += " FROM " + tableName + ";";
    try {
        SQLite::Statement stmt(*this->db, query);
        std::vector<std::vector<std::string>> results;
        while (stmt.executeStep()) {
            std::vector<std::string> row;
            for (size_t i = 0; i < columnNames.size(); i++) {
                row.push_back(stmt.getColumn(i).getText());
            }
            results.push_back(row);
        }
        return results;
    } catch (std::exception& e) {
        this->lastError = e.what();
        return {};
    }
}

/**
 * @brief Select data from a table. Selects all columns from the table and all rows in the table
 *
 * @param tableName
 * @return std::vector<std::vector<std::string>>
 */
std::vector<std::vector<std::string>> Database::selectData(const std::string& tableName)
{
    if (!this->isOpen()) {
        this->lastError = "Database is not open";
        return {};
    }
    std::string query = "SELECT * FROM " + tableName + ";";
    try {
        SQLite::Statement stmt(*this->db, query);
        std::vector<std::vector<std::string>> results;
        while (stmt.executeStep()) {
            std::vector<std::string> row;
            for (size_t i = 0; i < stmt.getColumnCount(); i++) {
                row.push_back(stmt.getColumn(i).getText());
            }
            results.push_back(row);
        }
        return results;
    } catch (std::exception& e) {
        this->lastError = e.what();
        return {};
    }
}

/**
 * @brief Check if a table exists in the database
 *
 * @param tableName
 * @return true
 * @return false
 */
bool Database::tableExists(const std::string& tableName)
{
    if (!this->isOpen()) {
        this->lastError = "Database is not open";
        return false;
    }
    std::string query = "SELECT name FROM sqlite_master WHERE type='table' AND name='" + tableName + "';";
    try {
        SQLite::Statement stmt(*this->db, query);
        if (stmt.executeStep()) {
            return true;
        } else {
            return false;
        }
    } catch (std::exception& e) {
        this->lastError = e.what();
        return false;
    }
}

/**
 * @brief Check if a column exists in a table
 *
 * @param tableName
 * @param columnName
 * @return true
 * @return false
 */
bool Database::columnExists(const std::string& tableName, const std::string& columnName)
{
    if (!this->isOpen()) {
        this->lastError = "Database is not open";
        return false;
    }
    std::string query = "PRAGMA table_info(" + tableName + ");";
    try {
        SQLite::Statement stmt(*this->db, query);
        while (stmt.executeStep()) {
            if (stmt.getColumn(1).getText() == columnName) {
                return true;
            }
        }
        return false;
    } catch (std::exception& e) {
        this->lastError = e.what();
        return false;
    }
}

/**
 * @brief Check if a row exists in a table
 *
 * @param tableName
 * @param whereClause
 * @return true
 * @return false
 */
bool Database::rowExists(const std::string& tableName, const std::string& whereClause)
{
    if (!this->isOpen()) {
        this->lastError = "Database is not open";
        return false;
    }
    std::string query = "SELECT * FROM " + tableName + " WHERE " + whereClause + ";";
    try {
        SQLite::Statement stmt(*this->db, query);
        if (stmt.executeStep()) {
            return true;
        } else {
            return false;
        }
    } catch (std::exception& e) {
        this->lastError = e.what();
        return false;
    }
}

/**
 * @brief Open the database
 *
 * @return true
 * @return false
 */
bool Database::open()
{
    if (this->isOpen()) {
        this->lastError = "Database is already open";
        return false;
    }
    try {
        this->db = std::make_shared<SQLite::Database>(this->dbPath, SQLite::OPEN_READWRITE);
        this->openFlag = true;
        return true;
    } catch (std::exception& e) {
        this->lastError = e.what();
        return false;
    }
}

/**
 * @brief Close the database
 *
 * @return true
 * @return false
 */
bool Database::close()
{
    if (!this->isOpen()) {
        this->lastError = "Database is not open";
        return false;
    }
    try {
        // delete this->db;
        this->openFlag = false;
        return true;
    } catch (std::exception& e) {
        this->lastError = e.what();
        return false;
    }
}

/**
 * @brief Check if the database is open
 *
 * @return true
 * @return false
 */
bool Database::isOpen()
{
    return this->openFlag;
}

/**
 * @brief Get the path of the database
 *
 * @return std::string
 */
std::string Database::getDBPath()
{
    return this->dbPath;
}

/**
 * @brief Get the name of the database
 *
 * @return std::string
 */
std::string Database::getDBName()
{
    return this->dbName;
}

/**
 * @brief Get the last error message
 *
 * @return std::string
 */
std::string Database::getLastError()
{
    return this->lastError;
}

/**
 * @brief Create a database
 *
 * @param dbPath
 * @return true
 * @return false
 */
bool Database::createDB(const std::string& dbPath)
{
    std::filesystem::path path(dbPath);
    if (std::filesystem::exists(path)) {
        return true;
    }
    if (!std::filesystem::exists(path.parent_path()) && !std::filesystem::create_directories(path.parent_path()))
        return false;
    try {
        SQLite::Database db(dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        return true;
    } catch (std::exception& e) {
        this->lastError = e.what();
        return false;
    }
}

/**
 * @brief Select a blob from a table
 *
 * @param tableName
 * @param columnName
 * @param whereClause
 * @param size
 * @return char*
 */
char* Database::selectBlob(const std::string& tableName, const std::string& columnName, const std::string& whereClause, int& size)
{
    if (!this->isOpen()) {
        this->lastError = "Database is not open";
        return nullptr;
    }
    std::string query = "SELECT " + columnName + " FROM " + tableName + " WHERE " + whereClause + ";";
    try {
        SQLite::Statement stmt(*this->db, query);
        if (stmt.executeStep()) {
            size = stmt.getColumn(0).size();
            char* blob = new char[size];
            // use std::copy to copy the data from the blob to the char array
            std::copy(static_cast<const char*>(stmt.getColumn(0).getBlob()), static_cast<const char*>(stmt.getColumn(0).getBlob()) + size, blob);
            return blob;
        } else {
            size = 0;
            return nullptr;
        }
    } catch (std::exception& e) {
        this->lastError = e.what();
        size = 0;
        return nullptr;
    }
}

/**
 * @brief Select a blob from a table
 *
 * @param tableName
 * @param columnName
 * @param whereClause
 * @return std::string
 */
std::string Database::selectBlobString(const std::string& tableName, const std::string& columnName, const std::string& whereClause)
{
    if (!this->isOpen()) {
        this->lastError = "Database is not open";
        return "";
    }
    std::string query = "SELECT " + columnName + " FROM " + tableName + " WHERE " + whereClause + ";";
    try {
        SQLite::Statement stmt(*this->db, query);
        if (stmt.executeStep()) {
            return stmt.getColumn(0).getText();
        } else {
            return "";
        }
    } catch (std::exception& e) {
        this->lastError = e.what();
        return "";
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Construct a new CubeDatabaseManager object. This object creates the databases and tables defined in the dbDefs vector.
 * If the .db file does not exist, it will be created.
 * If the database or table already exists, it will not be created. If the table does not exist, it will be created.
 *
 */
CubeDatabaseManager::CubeDatabaseManager()
{
    for (size_t i = 0; i < DB_NS::dbDefs.size(); i++) {
        if (!Database(DB_NS::dbDefs[i].path).createDB(DB_NS::dbDefs[i].path)) {
            CubeLog::error("Failed to create database: " + DB_NS::dbDefs[i].path);
        }
        CubeLog::info("Database created or exists: " + DB_NS::dbDefs[i].path);
        // create the tables
        for (size_t j = 0; j < DB_NS::dbDefs[i].tables.size(); j++) {
            Database db(DB_NS::dbDefs[i].path);
            if (db.open()) {
                CubeLog::info("Database opened: " + DB_NS::dbDefs[i].path);
                for (size_t k = 0; k < DB_NS::dbDefs[i].tables[j].columnNames.size(); k++) {
                    if (db.tableExists(DB_NS::dbDefs[i].tables[j].name)) {
                        if (!db.columnExists(DB_NS::dbDefs[i].tables[j].name, DB_NS::dbDefs[i].tables[j].columnNames[k])) {
                            if (!db.createTable(DB_NS::dbDefs[i].tables[j].name, DB_NS::dbDefs[i].tables[j].columnNames, DB_NS::dbDefs[i].tables[j].columnTypes, DB_NS::dbDefs[i].tables[j].columnUnique)) {
                                CubeLog::error("Failed to create table: " + DB_NS::dbDefs[i].tables[j].name);
                                CubeLog::error("Last error: " + db.getLastError());
                            } else {
                                CubeLog::info("Table created: " + DB_NS::dbDefs[i].tables[j].name);
                            }
                        } else {
                            CubeLog::info("Table exists: " + DB_NS::dbDefs[i].tables[j].name);
                        }
                    } else {
                        if (!db.createTable(DB_NS::dbDefs[i].tables[j].name, DB_NS::dbDefs[i].tables[j].columnNames, DB_NS::dbDefs[i].tables[j].columnTypes, DB_NS::dbDefs[i].tables[j].columnUnique)) {
                            CubeLog::error("Failed to create table: " + DB_NS::dbDefs[i].tables[j].name);
                            CubeLog::error("Last error: " + db.getLastError());
                        } else {
                            CubeLog::info("Table created: " + DB_NS::dbDefs[i].tables[j].name);
                        }
                    }
                }
                db.close();
            } else {
                CubeLog::error("Failed to open database: " + DB_NS::dbDefs[i].path);
                CubeLog::error("Last error: " + db.getLastError());
            }
        }
        this->addDatabase(DB_NS::dbDefs[i].path);
        this->getDatabase(DB_NS::dbDefs[i].name)->setUniqueColumns(DB_NS::dbDefs[i].tables[0].columnNames, DB_NS::dbDefs[i].tables[0].columnUnique);
    }
    dbThread = std::jthread(&CubeDatabaseManager::dbWorker, this);
    dbStopToken = dbThread.get_stop_token();
    std::lock_guard<std::mutex> lock(this->dbMutex);
    this->isReady = true;
}

/**
 * @brief Destroy the CubeDatabaseManager object. Closes all databases.
 *
 */
CubeDatabaseManager::~CubeDatabaseManager()
{
    CubeLog::info("CubeDatabaseManager closing");
    this->closeAll();
    std::stop_token st = dbThread.get_stop_token();
    dbThread.request_stop();
    dbThread.join();
}

/**
 * @brief Add a database to the database manager
 *
 * @param dbPath
 */
void CubeDatabaseManager::addDatabase(const std::string& dbPath)
{
    Database* db = new Database(dbPath);
    if (db->open()) {
        this->databases.push_back(db);
    } else {
        CubeLog::error("Failed to open database: " + dbPath);
        delete db;
    }
}

/**
 * @brief Get a database by name
 *
 * @param dbName
 * @return Database*
 */
Database* CubeDatabaseManager::getDatabase(const std::string& dbName)
{
    for (size_t i = 0; i < this->databases.size(); i++) {
        if (this->databases[i]->getDBName() == dbName) {
            return this->databases[i];
        }
    }
    throw std::runtime_error("Database not found");
}

/**
 * @brief Remove a database by name
 *
 * @param dbName
 * @return true
 * @return false
 */
bool CubeDatabaseManager::removeDatabase(const std::string& dbName)
{
    for (size_t i = 0; i < this->databases.size(); i++) {
        if (this->databases[i]->getDBName() == dbName) {
            this->databases[i]->close();
            delete this->databases[i];
            this->databases.erase(this->databases.begin() + i);
            return true;
        }
    }
    return false;
}

/**
 * @brief Get all databases
 *
 * @return std::vector<Database*>
 */
std::vector<Database*> CubeDatabaseManager::getDatabases()
{
    return this->databases;
}

/**
 * @brief Close all databases
 *
 */
void CubeDatabaseManager::closeAll()
{
    for (size_t i = 0; i < this->databases.size(); i++) {
        this->databases[i]->close();
        delete this->databases[i];
    }
    this->databases.clear();
}

/**
 * @brief Open all databases
 *
 */
void CubeDatabaseManager::openAll()
{
    for (size_t i = 0; i < this->databases.size(); i++) {
        if (!this->databases[i]->isOpen()) {
            if (!this->databases[i]->open()) {
                CubeLog::error("Failed to open database: " + this->databases[i]->getDBPath());
            }
        }
    }
}

/**
 * @brief Close a database by name
 *
 * @param dbName
 */
void CubeDatabaseManager::closeDatabase(const std::string& dbName)
{
    for (size_t i = 0; i < this->databases.size(); i++) {
        if (this->databases[i]->getDBName() == dbName) {
            this->databases[i]->close();
            delete this->databases[i];
            this->databases.erase(this->databases.begin() + i);
            return;
        }
    }
}

/**
 * @brief Open a database by name
 *
 * @param dbName
 * @return true
 * @return false
 */
bool CubeDatabaseManager::openDatabase(const std::string& dbName)
{
    for (size_t i = 0; i < this->databases.size(); i++) {
        if (this->databases[i]->getDBName() == dbName) {
            if (!this->databases[i]->isOpen()) {
                if (!this->databases[i]->open()) {
                    CubeLog::error("Failed to open database: " + this->databases[i]->getDBPath());
                    return false;
                } else {
                    CubeLog::info("Database opened: " + this->databases[i]->getDBPath());
                    return true;
                }
            } else {
                CubeLog::error("Database is already open: " + this->databases[i]->getDBPath());
                return true;
            }
        }
    }
    CubeLog::error("Database not found: " + dbName);
    return false;
}

void CubeDatabaseManager::dbWorker()
{
    while (true) {
        while (this->dbQueue.size() > 0) {
            auto task = this->dbQueue.pop();
            task();
        }
        genericSleep(100);
        if (dbStopToken.stop_requested())
            break;
    }
}

void CubeDatabaseManager::addDbTask(std::function<void()> task)
{
    this->dbQueue.push(task);
}

bool CubeDatabaseManager::isDatabaseManagerReady()
{
    std::lock_guard<std::mutex> lock(this->dbMutex);
    return this->isReady;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Construct a new BlobsManager object
 *
 * @param dbManager
 * @param dbPath
 */
BlobsManager::BlobsManager(std::shared_ptr<CubeDatabaseManager> dbManager, const std::string& dbPath)
{
    this->dbManager = dbManager;
    std::lock_guard<std::mutex> lock(this->blobsMutex);
    this->isReady = true;
}

/**
 * @brief Destroy the BlobsManager object
 *
 */
BlobsManager::~BlobsManager()
{
    CubeLog::info("BlobsManager closing");
}

/**
 * @brief Add a blob to a table
 *
 * @param tableName
 * @param blob
 * @param ownerID
 * @return true
 * @return false
 */
int BlobsManager::addBlob(const std::string& tableName, const std::string& blob, const std::string& ownerID)
{
    if (!isInBlobsTableNames(tableName)) {
        CubeLog::error("Invalid table name: " + tableName);
        return -1;
    }
    Database* db = this->dbManager->getDatabase("blobs");
    if (db->open()) {
        long id = db->insertData(tableName, { "blob", "owner_id" }, { blob, ownerID });
        if (id > -1) {
            db->close();
            return id;
        } else {
            CubeLog::error("Failed to add blob to table: " + tableName);
            CubeLog::error("Last error: " + db->getLastError());
            db->close();
            return -1;
        }
    } else {
        CubeLog::error("Failed to open database: blobs.db");
        CubeLog::error("Last error: " + db->getLastError());
        if (db->getLastError() == "Database is already open")
            return true;
        return -1;
    }
}

/**
 * @brief Remove a blob from a table
 *
 * @param tableName
 * @param blob
 * @param ownerID
 * @return true
 * @return false
 */
bool BlobsManager::removeBlob(const std::string& tableName, const std::string& ownerID, int id)
{
    if (!isInBlobsTableNames(tableName)) {
        CubeLog::error("Invalid table name: " + tableName);
        return false;
    }
    Database* db = this->dbManager->getDatabase("blobs.db");
    if (db->open()) {
        if (db->deleteData(tableName, "id = " + std::to_string(id) + " AND owner_id = '" + ownerID + "'")) {
            db->close();
            return true;
        } else {
            CubeLog::error("Failed to remove blob from table: " + tableName);
            CubeLog::error("Last error: " + db->getLastError());
            db->close();
            return false;
        }
    } else {
        CubeLog::error("Failed to open database: blobs.db");
        CubeLog::error("Last error: " + db->getLastError());
        return false;
    }
}

/**
 * @brief Get a blob from a table
 *
 * @param tableName
 * @param ownerID
 * @param id
 * @return std::string
 */
std::string BlobsManager::getBlobString(const std::string& tableName, const std::string& ownerID, int id)
{
    if (!isInBlobsTableNames(tableName)) {
        CubeLog::error("Invalid table name: " + tableName);
        return "";
    }
    Database* db = this->dbManager->getDatabase("blobs.db");
    if (db->open()) {
        std::vector<std::vector<std::string>> result = db->selectData(tableName, { "blob" }, "id = " + std::to_string(id) + " AND owner_id = '" + ownerID + "'");
        db->close();
        if (result.size() > 0) {
            return result[0][0];
        } else {
            return "";
        }
    } else {
        CubeLog::error("Failed to open database: blobs.db");
        CubeLog::error("Last error: " + db->getLastError());
        return "";
    }
}

/**
 * @brief Get a blob from a table
 *
 * @param tableName
 * @param ownerID
 * @param id
 * @param size
 * @return char*
 */
char* BlobsManager::getBlobChars(const std::string& tableName, const std::string& ownerID, int id, int& size)
{
    if (!isInBlobsTableNames(tableName)) {
        CubeLog::error("Invalid table name: " + tableName);
        return nullptr;
    }
    Database* db = this->dbManager->getDatabase("blobs.db");
    if (db->open()) {
        std::vector<std::vector<std::string>> result = db->selectData(tableName, { "blob" }, "id = " + std::to_string(id) + " AND owner_id = '" + ownerID + "'");
        db->close();
        if (result.size() > 0) {
            size = result[0][0].size();
            char* blob = new char[size];
            for (size_t i = 0; i < size; i++) {
                blob[i] = result[0][0][i];
            }
            return blob;
        } else {
            size = 0;
            return nullptr;
        }
    } else {
        CubeLog::error("Failed to open database: blobs.db");
        CubeLog::error("Last error: " + db->getLastError());
        size = 0;
        return nullptr;
    }
}

/**
 * @brief Update a blob in a table
 *
 * @param tableName
 * @param blob
 * @param ownerID
 * @param id
 * @return true
 * @return false
 */
bool BlobsManager::updateBlob(const std::string& tableName, const std::string& blob, const std::string& ownerID, int id)
{
    if (!isInBlobsTableNames(tableName)) {
        CubeLog::error("Invalid table name: " + tableName);
        return false;
    }
    Database* db = this->dbManager->getDatabase("blobs.db");
    if (db->open()) {
        if (db->updateData(tableName, { "blob" }, { blob }, "id = " + std::to_string(id) + " AND owner_id = '" + ownerID + "'")) {
            db->close();
            return true;
        } else {
            CubeLog::error("Failed to update blob in table: " + tableName);
            CubeLog::error("Last error: " + db->getLastError());
            db->close();
            return false;
        }
    } else {
        CubeLog::error("Failed to open database: blobs.db");
        CubeLog::error("Last error: " + db->getLastError());
        return false;
    }
}

/**
 * @brief Add a blob to a table
 *
 * @param tableName
 * @param blob
 * @param ownerID
 * @return true
 * @return false
 */
bool BlobsManager::updateBlob(const std::string& tableName, char* blob, int size, const std::string& ownerID, int id)
{
    if (!isInBlobsTableNames(tableName)) {
        CubeLog::error("Invalid table name: " + tableName);
        return false;
    }
    Database* db = this->dbManager->getDatabase("blobs.db");
    if (db->open()) {
        std::string blobStr = "";
        for (size_t i = 0; i < size; i++) {
            blobStr += blob[i];
        }
        if (db->updateData(tableName, { "blob" }, { blobStr }, "id = " + std::to_string(id) + " AND owner_id = '" + ownerID + "'")) {
            db->close();
            return true;
        } else {
            CubeLog::error("Failed to update blob in table: " + tableName);
            CubeLog::error("Last error: " + db->getLastError());
            db->close();
            return false;
        }
    } else {
        CubeLog::error("Failed to open database: blobs.db");
        CubeLog::error("Last error: " + db->getLastError());
        return false;
    }
}

/**
 * @brief Add a blob to a table
 *
 * @param tableName
 * @param blob
 * @param ownerID
 * @return int -  the id of the blob
 */
int BlobsManager::addBlob(const std::string& tableName, char* blob, int size, const std::string& ownerID)
{
    if (!isInBlobsTableNames(tableName)) {
        CubeLog::error("Invalid table name: " + tableName);
        return -1;
    }
    Database* db = this->dbManager->getDatabase("blobs.db");
    if (db->open()) {
        long id = db->insertData(tableName, { "blob", "owner_id" }, { blob, ownerID });
        if (id > -1) {
            db->close();
            return id;
        } else {
            CubeLog::error("Failed to add blob to table: " + tableName);
            CubeLog::error("Last error: " + db->getLastError());
            db->close();
            return -1;
        }
    } else {
        CubeLog::error("Failed to open database: blobs.db");
        CubeLog::error("Last error: " + db->getLastError());
        return -1;
    }
}

bool BlobsManager::isBlobsManagerReady()
{
    std::lock_guard<std::mutex> lock(this->blobsMutex);
    return this->isReady;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool sanitizeString(std::string& str)
{
    if (
        str.find("'") != std::string::npos || str.find("\"") != std::string::npos || str.find("\\") != std::string::npos || str.find(";") != std::string::npos || str.find("--") != std::string::npos || str.find("/*") != std::string::npos || str.find("*/") != std::string::npos || str.find("DROP") != std::string::npos || str.find("CREATE") != std::string::npos || str.find("ALTER") != std::string::npos || str.find("INSERT") != std::string::npos || str.find("UPDATE") != std::string::npos || str.find("DELETE") != std::string::npos || str.find("SELECT") != std::string::npos || str.find("FROM") != std::string::npos || str.find("WHERE") != std::string::npos || str.find("AND") != std::string::npos || str.find("OR") != std::string::npos || str.find("LIKE") != std::string::npos || str.find("IN") != std::string::npos || str.find("BETWEEN") != std::string::npos || str.find("IS") != std::string::npos || str.find("NULL") != std::string::npos || str.find("ORDER") != std::string::npos || str.find("BY") != std::string::npos || str.find("GROUP") != std::string::npos || str.find("HAVING") != std::string::npos || str.find("LIMIT") != std::string::npos || str.find("OFFSET") != std::string::npos || str.find("JOIN") != std::string::npos || str.find("INNER") != std::string::npos || str.find("LEFT") != std::string::npos || str.find("RIGHT") != std::string::npos || str.find("OUTER") != std::string::npos || str.find("FULL") != std::string::npos || str.find("UNION") != std::string::npos || str.find("INTERSECT") != std::string::npos || str.find("EXCEPT") != std::string::npos)
        return false;
    return true;
}