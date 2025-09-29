#pragma once
#include <thread>
#include <cstdint>
#include <string>
#include <cstring>
#include <mutex>
#include <atomic>
#include <iostream>
#include <set>

class TLVServer {
    uint16_t port_;
    int listen_fd{-1};
    std::atomic<bool> running{true};
    std::atomic<int> active_clients{0};
    std::mutex sockets_mtx;
    std::set<int> client_fds;

    void close_fd(int& fd);
    
    // Returns string representation of client address [127.0.0.1:PORT]
    std::string address_string(const struct sockaddr_in& client_addr);

    // Handles a single client connection
    void handle_client(int client_fd, const struct sockaddr_in& client_addr);
    
public:
    explicit TLVServer(uint16_t port);
    ~TLVServer();

    // Starts the server, returns false on error
    bool start();

    // Main server loop, blocks until stop() is called
    // This is for accepting new connections and spawning client handlers
    void run();

    // Stops the server and closes all connections
    void stop();
};
