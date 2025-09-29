#include "client.hpp"
#include "tlv.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

TLVClient::TLVClient(const std::string& host, int port)
    : host_(host), port_(port), sock_(-1) {}

void TLVClient::connect_server() {
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0) {
        perror("socket");
        exit(1);
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);

    if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

    if (connect(sock_, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }
}

void TLVClient::close_server() {
    if (sock_ >= 0) {
        close(sock_);
        sock_ = -1;
    }
}

void TLVClient::send_tlv(std::uint16_t type, const std::string& value) {
    TLV tlv(type, value);
    auto bytes = tlv.serialize();
    send(sock_, bytes.data(), bytes.size(), 0);
}

void TLVClient::run_short() {
    connect_server();
    send_tlv(0xE110, ""); // Hello
    send_tlv(0xDA7A, std::string("\x01\x02\x03\x04\x05", 5));
    send_tlv(0x0B1E, ""); // Goodbye
    close_server();
}

void TLVClient::run_long(int duration, int interval) {
	int val=120;
    connect_server();
    send_tlv(0xE110, "");
    for (int t = 0; t < duration; t += interval) {
        std::string value(interval, 'A');
        send_tlv(0xDA7A, value);
        std::this_thread::sleep_for(std::chrono::seconds(interval));
    }
    send_tlv(0x0B1E, "");
    close_server();
}

void TLVClient::run_burst(int count, int size) {
    connect_server();
    send_tlv(0xE110, "");
    for (int i = 0; i < count; i++) {
        std::string value(size, 'B');
        send_tlv(0xDA7A, value);
    }
    send_tlv(0x0B1E, "");
    close_server();
}

void TLVClient::run_oversized() {
    connect_server();
    uint32_t big_len = 1u << 30; // 1GB
    TLV tlv(0xDA7A, std::string("X")); // dummy
    auto header = tlv.serialize_header(big_len);
    send(sock_, header.data(), header.size(), 0);
    close_server();
}

void TLVClient::run_truncated() {
    connect_server();
    TLV tlv(0xDA7A, "");
    auto header = tlv.serialize_header(100);
    send(sock_, header.data(), header.size(), 0);
    close_server();
}

void TLVClient::run_unknown() {
    connect_server();
    send_tlv(0x9999, "????");
    close_server();
}

void TLVClient::run_mixed() {
    connect_server();
    send_tlv(0xE110, "");
    send_tlv(0xDA7A, "1234");
    send_tlv(0x9999, "???"); // Unknown
    run_truncated();
    send_tlv(0x0B1E, "");
    close_server();
}

void TLVClient::run_large_payload() {
    connect_server();
    send_tlv(0xE110, "");
    
    // Send 1MB payload
    std::string large_data(10*1024 * 1024, 'L');
    send_tlv(0xDA7A, large_data);
    
    // Send 2MB payload
    std::string larger_data(2 * 1024 * 1024, 'M');
    send_tlv(0xDA7A, larger_data);
    
    send_tlv(0x0B1E, "");
    close_server();
}