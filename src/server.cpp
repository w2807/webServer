#include "server.h"

#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

const std::unordered_map<int, std::string> kReasonPhrases = {
    {200, "OK"},
    {404, "Not Found"},
    {400, "Bad Request"},
    {500, "Internal Server Error"},
};

auto httpserver::Server::parse_request(const std::string& request_str)
    -> Request {
    std::istringstream stream(request_str);
    Request request;
    std::string line;

    std::getline(stream, line);
    std::istringstream request_line(line);
    request_line >> request.method >> request.path;

    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    while (std::getline(stream, line) && !line.empty()) {
        const auto kPos = line.find(':');
        if (kPos != std::string::npos) {
            auto key = line.substr(0, kPos);
            auto value = line.substr(kPos + 1);
            if (!key.empty() && !value.empty()) {
                value.erase(0, value.find_first_not_of(' '));
                request.headers[std::move(key)] = std::move(value);
            }
        }
    }
    return request;
}

auto httpserver::Server::build_response(const Response& response)
    -> std::string {
    std::ostringstream stream;
    stream << "HTTP/1.1 " << response.status_code << ' '
           << ((kReasonPhrases.contains(response.status_code))
                   ? kReasonPhrases.at(response.status_code)
                   : "UNKNOWN")
           << "\r\n"
           << "Content-Type: " << response.content_type << "\r\n"
           << "Content-Length: " << response.body.size() << "\r\n"
           << "Connection: close\r\n"
           << "\r\n"
           << response.body;

    return stream.str();
}

void httpserver::Server::add_route(const std::string& path, Handler handler) {
    routes_[path] = std::move(handler);
}

void httpserver::Server::handle_connection(int client_fd) {
    constexpr int kBufferSize = 4096;
    std::string raw;
    raw.reserve(kBufferSize);

    std::array<char, kBufferSize> buffer;
    while (true) {
        const ssize_t kBytesReceived =
            read(client_fd, buffer.data(), kBufferSize);
        if (kBytesReceived < 0) {
            if (errno == EAGAIN) {
                break;
            }
            std::cerr << "Failed to read: " << strerror(errno) << '\n';
            close(client_fd);
            return;
        }
        if (kBytesReceived == 0) {
            std::cout << "Client " << client_fd << " disconnected." << '\n';
            close(client_fd);
            return;
        }
        raw.append(buffer.data(), kBytesReceived);
        if (raw.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }

    constexpr int kNotFound = 404;
    Response response;
    try {
        auto request = parse_request(raw);
        auto route_it = routes_.find(request.path);
        if (route_it != routes_.end()) {
            route_it->second(request, response);
        } else {
            response.status_code = kNotFound;
            response.content_type = "text/plain";
            response.body = "404 Not Found";
        }

        auto response_str = build_response(response);
        const ssize_t kBytesSent =
            write(client_fd, response_str.c_str(), response_str.size());
        if (kBytesSent < 0) {
            std::cerr << "Failed to write: " << strerror(errno) << '\n';
        } else {
            std::cout << "Response sent to client." << '\n';
        }
        close(client_fd);
    } catch (const std::exception& e) {
        constexpr int kInternalServerError = 500;
        std::cerr << "Error processing request: " << e.what() << '\n';
        response.status_code = kInternalServerError;
        response.content_type = "text/plain";
        response.body = "Internal Server Error";
        auto response_str = build_response(response);
        write(client_fd, response_str.c_str(), response_str.size());
        close(client_fd);
    }
}
