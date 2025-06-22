#include "sql.h"

#include <mysqlx/devapi/common.h>
#include <mysqlx/devapi/result.h>
#include <mysqlx/xdevapi.h>
#include <openssl/sha.h>

#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

mysql::SQL::SQL(const std::string& uri) : session_(uri) {
    try {
        session_.sql("CREATE DATABASE IF NOT EXISTS `Users`").execute();
        session_
            .sql(R"(CREATE TABLE IF NOT EXISTS `Users`.`users` (
            `id` INT AUTO_INCREMENT PRIMARY KEY,
            `username` VARCHAR(255) NOT NULL,
            `password` VARCHAR(255) NOT NULL
            ))")
            .execute();
        session_.sql("USE `Users`").execute();
    } catch (const mysqlx::Error& e) {
        std::cerr << "error SQL: " << e.what() << '\n';
        throw;
    }
}

auto mysql::SQL::search(const std::string& query) -> mysqlx::RowResult {
    try {
        mysqlx::SqlStatement statement = session_.sql(query);
        return statement.execute();
    } catch (const mysqlx::Error& e) {
        std::cerr << "error search: " << e.what() << '\n';
        throw;
    }
    return {};  // never reach
}

void mysql::SQL::insert(const std::string& query) {
    try {
        mysqlx::SqlStatement statement = session_.sql(query);
        statement.execute();
    } catch (const mysqlx::Error& e) {
        std::cerr << "error insert: " << e.what() << '\n';
        throw;
    }
}

void mysql::SQL::insert(const std::string& username,
                        const std::string& passwd) {
    try {
        std::string salted = passwd + kSalt;
        std::array<unsigned char, SHA256_DIGEST_LENGTH> hash;
        SHA256(reinterpret_cast<const unsigned char*>(salted.data()),
               salted.size(), hash.data());
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            oss << std::setw(2) << static_cast<int>(hash[i]);
        }
        const std::string kHash = oss.str();
        session_.sql("INSERT INTO `users` (username, password) VALUES (?, ?)")
            .bind(username, kHash)
            .execute();
    } catch (const mysqlx::Error& e) {
        std::cerr << "error insert: " << e.what() << '\n';
        throw;
    }
}