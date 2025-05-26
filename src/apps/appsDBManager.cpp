/*
 █████╗ ██████╗ ██████╗ ███████╗██████╗ ██████╗ ███╗   ███╗ █████╗ ███╗   ██╗ █████╗  ██████╗ ███████╗██████╗     ██████╗██████╗ ██████╗ 
██╔══██╗██╔══██╗██╔══██╗██╔════╝██╔══██╗██╔══██╗████╗ ████║██╔══██╗████╗  ██║██╔══██╗██╔════╝ ██╔════╝██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
███████║██████╔╝██████╔╝███████╗██║  ██║██████╔╝██╔████╔██║███████║██╔██╗ ██║███████║██║  ███╗█████╗  ██████╔╝   ██║     ██████╔╝██████╔╝
██╔══██║██╔═══╝ ██╔═══╝ ╚════██║██║  ██║██╔══██╗██║╚██╔╝██║██╔══██║██║╚██╗██║██╔══██║██║   ██║██╔══╝  ██╔══██╗   ██║     ██╔═══╝ ██╔═══╝ 
██║  ██║██║     ██║     ███████║██████╔╝██████╔╝██║ ╚═╝ ██║██║  ██║██║ ╚████║██║  ██║╚██████╔╝███████╗██║  ██║██╗╚██████╗██║     ██║     
╚═╝  ╚═╝╚═╝     ╚═╝     ╚══════╝╚═════╝ ╚═════╝ ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
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

#include "appsDBManager.h"

class DBManager {
public:
    // ctor connects to the admin DB and registers prepared statements
    DBManager(const std::string& connInfo)
        : adminConn(connInfo)
    {
        adminConn.prepare(
            "insert_mapping",
            "INSERT INTO app_mappings(id, db_name, username, password) "
            "VALUES($1, $2, $3, $4)");
        adminConn.prepare(
            "get_mapping_by_id",
            "SELECT db_name, username, password FROM app_mappings WHERE id = $1");
    }

    // 1) Generates random ID & password, 2) creates DB+user, 3) grants rights,
    // 4) stores mapping → returns that ID
    std::string createApp(const std::string& id)
    {
        std::string dbName = "appdb_" + id;
        std::string user = "user_" + id;
        std::string pass = generateRandomPassword();

        pqxx::work txn { adminConn };
        // txn.exec0("CREATE DATABASE " + txn.quote_name(dbName));
        txn.exec("CREATE DATABASE " + txn.quote_name(dbName)).no_rows();
        txn.exec("CREATE USER " + txn.quote_name(user)
               + " WITH PASSWORD " + txn.quote(pass))
            .no_rows();
        txn.exec("GRANT ALL PRIVILEGES ON DATABASE "
               + txn.quote_name(dbName)
               + " TO " + txn.quote_name(user))
            .no_rows();
        txn.exec(pqxx::prepped { "insert_mapping" }, pqxx::params { id, dbName, user, pass });
        txn.commit();

        return id;
    }

    // Fetches the stored conn-info for a given ID
    std::string getConnectionString(const std::string& id)
    {
        pqxx::nontransaction ntx { adminConn };
        auto result = ntx.exec(pqxx::prepped { "get_mapping_by_id" }, pqxx::params { id });
        if (result.empty())
            throw std::runtime_error("Unknown ID: " + id);

        auto row = result[0];
        auto db = row["db_name"].c_str();
        auto user = row["username"].c_str();
        auto pass = row["password"].c_str();

        // Standard libpq URI:
        return "postgresql://" + std::string(user) + ":" + std::string(pass) + "@127.0.0.1:5432/" + std::string(db);
    }

private:
    pqxx::connection adminConn;

    // 16-char random alphanumeric password
    std::string generateRandomPassword()
    {
        static constexpr char alpha[] = "0123456789"
                                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz";
        std::mt19937_64 rng { std::random_device {}() };
        std::uniform_int_distribution<> dist(0, sizeof(alpha) - 2);
        std::string s(16, '\0');
        for (auto& c : s)
            c = alpha[dist(rng)];
        return s;
    }
};

// TODO: add the interfaces for HTTP server