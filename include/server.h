#ifndef SERVER_H
#define SERVER_H

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

namespace httpserver {

constexpr int kConnectionOk = 200;
constexpr int kNotFound = 404;
constexpr int kRedirect = 302;
constexpr int kInternalServerError = 500;
constexpr int kMethodNotAllowed = 405;

enum class FileType : std::uint8_t { HTML, JPG, MP4, NONE };

auto get_type(const std::string& path) -> FileType;

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
    std::unordered_map<std::string, std::string> headers;
};

using Handler = std::function<void(const Request&, Response&)>;

class Server {
private:
    std::unordered_map<std::string, Handler> routes_;
    std::string root_dir_;

    static auto parse_request(const std::string& request_str) -> Request;
    auto get_response(const std::string& raw) -> Response;
    static auto write_response(int client_fd, const Response& response) -> bool;

public:
    explicit Server(std::string root_dir = "./assets")
        : root_dir_(std::move(root_dir)) {};

    void add_route(const std::string& path, Handler handler);
    auto handle_connection(int client_fd) -> bool;
};

}  // namespace httpserver

#endif  // SERVER_H
