#pragma once
#include <string>
#include <cstdint>

class TLVClient {
public:
    TLVClient(const std::string& host, int port);
    void run_short();
    void run_long(int duration, int interval);
    void run_burst(int count, int size);
    void run_oversized();
    void run_truncated();
    void run_unknown();
    void run_mixed();
    void run_large_payload();
private:
    std::string host_;
    int port_;
    int sock_;

    void connect_server();
    void close_server();
    void send_tlv(std::uint16_t type, const std::string& value);
};
