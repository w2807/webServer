#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#include "thread_pool.h"

auto main() -> int {
    const int kPort = 8000;
    const int kServerFd = socket(AF_INET, SOCK_STREAM, 0);
    if (kServerFd < 0) {
        std::cerr << "Failed to socket: " << strerror(errno) << '\n';
        return 1;
    }

    threadpool::ThreadPool pool;

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(kPort);

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
        return 1;
    }
    std::cout << "Server is listening on port " << kPort << '\n';

    const int kBufferSize = 4096;

    while (true) {
        sockaddr_in client_address{};
        socklen_t client_address_length = sizeof(client_address);
        const int kClientFd =
            accept(kServerFd, reinterpret_cast<sockaddr*>(&client_address),
                   &client_address_length);
        if (kClientFd < 0) {
            std::cerr << "Failed to accept: " << strerror(errno) << '\n';
            continue;
        }

        pool.enqueue([kClientFd, kBufferSize]() {
            std::array<char, kBufferSize> buffer{};
            const ssize_t kBytesReceive =
                read(kClientFd, buffer.data(), buffer.size());
            if (kBytesReceive < 0) {
                std::cerr << "Failed to read: " << strerror(errno) << '\n';
            }
            const std::string kResponse =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 13\r\n"
                "Connection: close\r\n"
                "\r\n"
                "Hello, World!";

            const ssize_t kBytesSent =
                write(kClientFd, kResponse.c_str(), kResponse.size());
            if (kBytesSent < 0) {
                std::cerr << "Failed to write: " << strerror(errno) << '\n';
            } else {
                std::cout << "Response sent to client." << '\n';
            }
            close(kClientFd);
        });
    }
    close(kServerFd);
    return 0;
}
