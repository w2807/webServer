#include <asm-generic/socket.h>
#include <fcntl.h>
#include <mysqlx/devapi/common.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include "server.h"
#include "sql.h"
#include "thread_pool.h"

namespace {

constexpr int kMaxEvents = 8192;

auto set_no_block(int file_descriptor) -> int {
    const int kFlags = fcntl(file_descriptor, F_GETFL, 0);
    return fcntl(file_descriptor, F_SETFL, kFlags | O_NONBLOCK);
}

auto set_server_socket(int port) -> int {
    const int kServerFd = socket(AF_INET, SOCK_STREAM, 0);
    if (kServerFd < 0 || set_no_block(kServerFd) < 0) {
        std::cerr << "Failed to socket: " << strerror(errno) << '\n';
        return -1;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    int opt = 1;
    setsockopt(kServerFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(kServerFd, reinterpret_cast<sockaddr*>(&server_address),
             sizeof(server_address)) < 0) {
        std::cerr << "Failed to bind: " << strerror(errno) << '\n';
        close(kServerFd);
        return 1;
    }
    if (listen(kServerFd, SOMAXCONN) < 0) {
        std::cerr << "Failed to listen: " << strerror(errno) << '\n';
        close(kServerFd);
        return -1;
    }

    std::cout << "Server is listening on port " << port << '\n';
    return kServerFd;
}

auto set_epoll(int listen_fd) -> int {
    const int kEpfd = epoll_create1(0);
    if (kEpfd < 0) {
        std::cerr << "Failed to create epoll: " << strerror(errno) << '\n';
        close(listen_fd);
        return -1;
    }

    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = listen_fd;

    if (epoll_ctl(kEpfd, EPOLL_CTL_ADD, listen_fd, &event) < 0) {
        std::cerr << "Failed to add server socket to epoll: " << strerror(errno)
                  << '\n';
        close(listen_fd);
        return -1;
    }
    return kEpfd;
}

void handle_new_client(int listen_fd, int epfd) {
    sockaddr_in client_address{};
    socklen_t client_length = sizeof(client_address);

    while (true) {
        const int kClientFd =
            accept(listen_fd, reinterpret_cast<sockaddr*>(&client_address),
                   &client_length);
        if (kClientFd < 0) {
            if (errno == EAGAIN) {
                break;
            }
            std::cerr << "Failed to accept: " << strerror(errno) << '\n';
            return;
        }
        if (set_no_block(kClientFd) < 0) {
            std::cerr << "Failed to set non-blocking: " << strerror(errno)
                      << '\n';
            close(kClientFd);
            return;
        }

        int opt = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

        epoll_event event{};
        event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
        event.data.fd = kClientFd;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, kClientFd, &event) < 0) {
            std::cerr << "Failed to add client socket to epoll: "
                      << strerror(errno) << '\n';
            close(kClientFd);
            return;
        }
    }
}

void handle_IO(int event_fd, threadpool::ThreadPool& pool,
               httpserver::Server& server, int epoll_fd) {
    pool.enqueue([event_fd, epoll_fd, &server] {
        if (server.handle_connection(event_fd)) {
            epoll_event event{};
            event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
            event.data.fd = event_fd;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_fd, &event) < 0) {
                std::cerr << "Failed to modify epoll event: " << strerror(errno)
                          << '\n';
                close(event_fd);
            }
        }
    });
}

void register_handler(const httpserver::Request& request,
                      httpserver::Response& response, mysql::SQL& sql) {
    if (request.method != "POST") {
        response.status_code = httpserver::kMethodNotAllowed;
        response.content_type = "text/plain";
        response.body = "Method Not Allowed";
        return;
    }
    if (!request.body.empty()) {
        const auto kUsernamePos = request.body.find("username=");
        const auto kPasswordPos = request.body.find("password=");
        if (kUsernamePos != std::string::npos &&
            kPasswordPos != std::string::npos) {
            const auto kUsername = request.body.substr(
                kUsernamePos + 9, kPasswordPos - kUsernamePos - 10);
            const auto kPassword = request.body.substr(
                kPasswordPos + 9, request.body.size() - kPasswordPos - 9);

#ifdef DEBUG
            std::cout << "username: " << kUsername << '\n';
            std::cout << "password: " << kPassword << '\n';
#endif

            try {
                sql.insert(kUsername, kPassword);
                response.status_code = httpserver::kConnectionOk;
                response.content_type = "text/plain";
                response.body = "User registered successfully!";
            } catch (const mysqlx::Error& e) {
                std::cerr << "SQL error: " << e.what() << '\n';
                response.status_code = httpserver::kInternalServerError;
                response.content_type = "text/plain";
                response.body = "Internal Server Error";
            }
        }
    }
}

void login_handler(const httpserver::Request& request,
                   httpserver::Response& response, mysql::SQL& sql) {
    if (request.method != "POST") {
        response.status_code = httpserver::kMethodNotAllowed;
        response.content_type = "text/plain";
        response.body = "Method Not Allowed";
        return;
    }
    if (!request.body.empty()) {
        const auto kUsernamePos = request.body.find("username=");
        const auto kPasswordPos = request.body.find("password=");
        if (kUsernamePos != std::string::npos &&
            kPasswordPos != std::string::npos) {
            const auto kUsername = request.body.substr(
                kUsernamePos + 9, kPasswordPos - kUsernamePos - 10);
            const auto kPassword = request.body.substr(
                kPasswordPos + 9, request.body.size() - kPasswordPos - 9);

#ifdef DEBUG
            std::cout << "username: " << kUsername << '\n';
            std::cout << "password: " << kPassword << '\n';
#endif
            try {
                auto result = sql.search(kUsername, kPassword);
                if (result) {
                    response.status_code = httpserver::kConnectionOk;
                    response.content_type = "text/plain";
                    response.body = "Login successful!";
                } else {
                    response.status_code = httpserver::kUnauthorized;
                    response.content_type = "text/plain";
                    response.body = "Invalid username or password.";
                }
            } catch (const mysqlx::Error& e) {
                std::cerr << "SQL error: " << e.what() << '\n';
                response.status_code = httpserver::kInternalServerError;
                response.content_type = "text/plain";
                response.body = "Internal Server Error";
            }
        }
    }
}

void add_route(httpserver::Server& server, mysql::SQL& sql) {
    server.add_route("/", [](auto&&, auto&& response) {
        response.status_code = httpserver::kRedirect;
        response.headers["location"] = "/index.html";
    });
    server.add_route("/hello", [](auto&&, auto&& response) {
        response.status_code = httpserver::kConnectionOk;
        response.content_type = "text/plain";
        response.body = "Hello from /hello!";
    });
    server.add_route("/goodbye", [](auto&&, auto&& response) {
        response.status_code = httpserver::kConnectionOk;
        response.content_type = "text/plain";
        response.body = "Goodbye!";
    });
    server.add_route("/error", [](auto&&, auto&& response) {
        throw std::runtime_error("error");
        response.status_code = httpserver::kConnectionOk;
        response.content_type = "text/plain";
        response.body = "Should be an error";
    });
    server.add_route("/api/register", [&](auto&& request, auto&& response) {
        register_handler(request, response, sql);
    });
    server.add_route("/api/login", [&](auto&& request, auto&& response) {
        login_handler(request, response, sql);
    });
}

auto check_timeout(int server_fd, int ep_fd,
                   std::array<epoll_event, kMaxEvents>& events,
                   std::chrono::steady_clock::time_point last_active,
                   std::chrono::seconds timeout) -> bool {
    auto now = std::chrono::steady_clock::now();
    if (now - last_active < timeout) {
        return false;
    }

    std::cout << "Server inactive for "
              << std::chrono::duration_cast<std::chrono::seconds>(now -
                                                                  last_active)
                     .count()
              << " seconds, shutting down.\n";

    for (auto& event : events) {
        if (event.data.fd != server_fd) {
            close(event.data.fd);
        }
    }
    close(ep_fd);
    return true;
}

}  // namespace

auto main() -> int {
    constexpr int kPort = 8000;

    threadpool::ThreadPool pool;
    httpserver::Server server;
    mysql::SQL sql("mysqlx://wkz2807:114514@localhost:33060");

    add_route(server, sql);

    const int kServerFd = set_server_socket(kPort);
    if (kServerFd < 0) {
        return 1;
    }

    const int kEpfd = set_epoll(kServerFd);
    if (kEpfd < 0) {
        close(kServerFd);
        return 1;
    }

    std::array<epoll_event, kMaxEvents> events{};

    constexpr int kEpollTimeoutMs = 10000;
    constexpr std::chrono::seconds kServerDownTimeout(300);
    auto last_active = std::chrono::steady_clock::now();

    while (true) {
        const int kEventCount =
            epoll_wait(kEpfd, events.data(), kMaxEvents, kEpollTimeoutMs);
        if (kEventCount < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "Failed to wait for events: " << strerror(errno)
                      << '\n';
            close(kEpfd);
            close(kServerFd);
            return 1;
        }
        if (kEventCount > 0) {
            last_active = std::chrono::steady_clock::now();
            for (int i = 0; i < kEventCount; i++) {
                const int kEventFd = events[i].data.fd;
                if (kEventFd == kServerFd) {
                    handle_new_client(kServerFd, kEpfd);
                } else if ((events[i].events & EPOLLIN) != 0U) {
                    handle_IO(kEventFd, pool, server, kEpfd);
                }
            }
        } else if (check_timeout(kServerFd, kEpfd, events, last_active,
                                 kServerDownTimeout)) {
            break;
        }
    }
    close(kServerFd);
    return 0;
}
