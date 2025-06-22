#ifndef SQL_H
#define SQL_H

#include <mysqlx/xdevapi.h>

#include <string>

namespace mysql {

constexpr std::string kSalt = "wkz2807";

class SQL {
public:
    explicit SQL(const std::string& uri);

    auto search(const std::string& query) -> mysqlx::RowResult;
    void insert(const std::string& query);
    void insert(const std::string& username, const std::string& passwd);

private:
    mysqlx::Session session_;
};

}  // namespace mysql

#endif  // SQL_H
