#include <asm-generic/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#include "thread_pool.h"

namespace {

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
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
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

        epoll_event event{};
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = kClientFd;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, kClientFd, &event) < 0) {
            std::cerr << "Failed to add client socket to epoll: "
                      << strerror(errno) << '\n';
            close(kClientFd);
            return;
        }
    }
}

void handle_IO(int event_fd, threadpool::ThreadPool& pool) {
    constexpr int kBufferSize = 4096;

    pool.enqueue([event_fd] mutable {
        std::array<char, kBufferSize> buffer{};

        while (true) {
            const ssize_t kBytesReceive =
                read(event_fd, buffer.data(), kBufferSize);
            if (kBytesReceive < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                std::cerr << "Failed to read: " << strerror(errno) << '\n';
                close(event_fd);
                return;
            }
            if (kBytesReceive == 0) {
                close(event_fd);
                return;
            }
        }

        const std::string kResponse =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Hello, World!";

        const ssize_t kBytesSent =
            write(event_fd, kResponse.c_str(), kResponse.size());
        if (kBytesSent < 0) {
            std::cerr << "Failed to write: " << strerror(errno) << '\n';
        } else {
            std::cout << "Response sent to client." << '\n';
        }
        close(event_fd);
    });
}

}  // namespace

auto main() -> int {
    const int kPort = 8000;

    threadpool::ThreadPool pool;

    const int kServerFd = set_server_socket(kPort);
    if (kServerFd < 0) {
        return 1;
    }

    const int kEpfd = set_epoll(kServerFd);
    if (kEpfd < 0) {
        close(kServerFd);
        return 1;
    }

    constexpr int kMaxEvents = 1024;
    std::array<epoll_event, kMaxEvents> events{};

    while (true) {
        const int kEventCount =
            epoll_wait(kEpfd, events.data(), kMaxEvents, -1);
        for (int i = 0; i < kEventCount; i++) {
            const int kEventFd = events[i].data.fd;
            if (kEventFd == kServerFd) {
                handle_new_client(kServerFd, kEpfd);
            } else if ((events[i].events & EPOLLIN) != 0U) {
                handle_IO(kEventFd, pool);
            }
        }
    }
    close(kServerFd);
    return 0;
}
