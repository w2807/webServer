#ifndef SERVER_H
#define SERVER_H

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

namespace httpserver {

constexpr int kConnectionOk = 200;
constexpr int kNotFound = 404;

struct Request {
    std::string method;
    std::string path;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct Response {
    int status_code;
    std::string content_type;
    std::string body;
};

using Handler = std::function<void(const Request&, Response&)>;

class Server {
private:
    std::unordered_map<std::string, Handler> routes_;
    std::string root_dir_;

    static auto parse_request(const std::string& request_str) -> Request;
    static auto build_response(const Response& response) -> std::string;

public:
    explicit Server(std::string root_dir = "./assets")
        : root_dir_(std::move(root_dir)) {};

    void add_route(const std::string& path, Handler handler);
    void handle_connection(int client_fd);
};

}  // namespace httpserver

#endif  // SERVER_H
