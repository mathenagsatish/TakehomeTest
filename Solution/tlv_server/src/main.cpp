#include "server.hpp"
#include <iostream>
#include <csignal>
#include <memory>

std::unique_ptr<TLVServer> g_server;

void signalHandler(int) {
    if (g_server) g_server->stop();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }
    int port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port\n";
        return 1;
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    g_server = std::make_unique<TLVServer>(static_cast<uint16_t>(port));
    if (!g_server->start()) {
        std::cerr << "Failed to start server\n";
        return 1;
    }
    g_server->run();
    return 0;
}
