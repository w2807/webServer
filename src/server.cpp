#include "server.h"

#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

const std::unordered_map<int, std::string> kReasonPhrases = {
    {200, "OK"},
    {404, "Not Found"},
    {400, "Bad Request"},
    {302, "Found"},
    {500, "Internal Server Error"},
};

auto httpserver::Server::parse_request(const std::string& request_str)
    -> Request {
#ifdef DEBUG
    std::cout << "request: " << request_str << '\n';
#endif

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

    if (request.method == "POST") {
        std::getline(stream, request.body);
        if (!request.body.empty() && request.body.back() == '\r') {
            request.body.pop_back();
        }
    }

#ifdef DEBUG
    std::cout << "method: " << request.method << '\n';
    std::cout << "path: " << request.path << '\n';
    for (const auto& [key, value] : request.headers) {
        std::cout << "header: " << key << ": " << value << '\n';
    }
    std::cout << "body: " << request.body << '\n';
#endif

    return request;
}

// auto httpserver::Server::build_response(const Response& response)
//     -> std::string {
//     std::ostringstream stream;
//     stream << "HTTP/1.1 " << response.status_code << ' '
//            << ((kReasonPhrases.contains(response.status_code))
//                    ? kReasonPhrases.at(response.status_code)
//                    : "UNKNOWN")
//            << "\r\n";

//     for (const auto& [key, value] : response.headers) {
//         stream << key << ": " << value << "\r\n";
//     }

//     stream << "Content-Type: " << response.content_type << "\r\n"
//            << "Content-Length: " << response.body.size() << "\r\n"
//            << "Connection: close\r\n"
//            << "\r\n"
//            << response.body;

//     return stream.str();
// }

auto httpserver::Server::write_response(int client_fd, const Response& response)
    -> bool {
    std::ostringstream stream;
    stream << "HTTP/1.1 " << response.status_code << " "
           << ((kReasonPhrases.contains(response.status_code)
                    ? kReasonPhrases.at(response.status_code)
                    : "UNKNOWN"))
           << "\r\n"
           << "Content-Type: " << response.content_type << "\r\n"
           << "Content-Length: " << response.body.size() << "\r\n"
           << "Connection: keep-alive" << "\r\n";
    for (const auto& [key, value] : response.headers) {
        stream << key << ": " << value << "\r\n";
    }
    stream << "\r\n";

#ifdef DEBUG
    std::cout << "response: " << stream.str() << '\n';
#endif

    const std::string kHeaderStr = stream.str();
    const auto kHeaderSize = kHeaderStr.size();
    size_t header_sent = 0;
    while (header_sent < kHeaderSize) {
        const auto kSentOnce = write(client_fd, kHeaderStr.data() + header_sent,
                                     kHeaderSize - header_sent);
        if (kSentOnce < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            std::cerr << "Failed to write header: " << strerror(errno) << '\n';
            return false;
        }
        header_sent += kSentOnce;
    }

    size_t body_sent = 0;
    const auto kBodySize = response.body.size();

#ifdef DEBUG
    std::cout << "Response body size: " << kBodySize << '\n';
#endif

    while (body_sent < kBodySize) {
        const auto kSentOnce = write(
            client_fd, response.body.data() + body_sent, kBodySize - body_sent);
        if (kSentOnce < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            std::cerr << "Failed to write body: " << strerror(errno) << '\n';
            return false;
        }
        body_sent += kSentOnce;
    }
    return true;
}

void httpserver::Server::add_route(const std::string& path, Handler handler) {
    routes_[path] = std::move(handler);
}

auto httpserver::Server::get_response(const std::string& raw) -> Response {
    Response response;
    auto request = parse_request(raw);
    auto route_it = routes_.find(request.path);
    if (route_it != routes_.end()) {
        route_it->second(request, response);
    } else {
        const std::filesystem::path kFilePath = root_dir_ + request.path;
        if (std::filesystem::exists(kFilePath) &&
            std::filesystem::is_regular_file(kFilePath)) {
            response.status_code = kConnectionOk;
            const FileType kFileType = get_type(kFilePath.string());

#ifdef DEBUG
            std::cout << "file path: " << kFilePath.string() << '\n';
#endif

            if (kFileType == FileType::HTML) {
                response.content_type = "text/html";
            } else if (kFileType == FileType::JPG) {
                response.content_type = "image/jpeg";
            } else if (kFileType == FileType::MP4) {
                response.content_type = "video/mp4";
            } else {
                response.content_type = "application/octet-stream";
            }
            std::ifstream file(kFilePath, std::ios::binary);
            if (file) {
                response.body.assign(std::istreambuf_iterator<char>(file),
                                     std::istreambuf_iterator<char>());
            } else {
                std::cerr << "Failed to open file: " << kFilePath.string()
                          << '\n';
            }
        } else {
            response.status_code = kNotFound;
            response.content_type = "text/plain";
            response.body = "404 Not Found";
        }
    }

    return response;
}

auto httpserver::Server::handle_connection(int client_fd) -> bool {
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
            return false;
        }
        if (kBytesReceived == 0) {
            std::cout << "Client " << client_fd << " disconnected." << '\n';
            close(client_fd);
            return false;
        }
        raw.append(buffer.data(), kBytesReceived);
    }

    Response response;
    try {
        response = get_response(raw);
        if (!write_response(client_fd, response)) {
            close(client_fd);
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing request: " << e.what() << '\n';
        response.status_code = kInternalServerError;
        response.content_type = "text/plain";
        response.body = "Internal Server Error";
        write_response(client_fd, response);
    }
    return true;
}

auto httpserver::get_type(const std::string& path) -> FileType {
    const auto kPos = path.find_last_of('.');
    if (kPos == std::string::npos) {
        return FileType::NONE;
    }
    const auto kExt = path.substr(kPos + 1);
    if (kExt == "html" || kExt == "htm") {
        return FileType::HTML;
    }
    if (kExt == "jpg" || kExt == "jpeg") {
        return FileType::JPG;
    }
    if (kExt == "mp4") {
        return FileType::MP4;
    }
    return FileType::NONE;
}
