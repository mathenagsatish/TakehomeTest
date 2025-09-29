#include "tlv.hpp"
#include <arpa/inet.h>
#include <cstring>

TLV::TLV(uint16_t type, const std::string& value)
    : type_(type), value_(value) {}

std::vector<uint8_t> TLV::serialize() const {
    std::vector<uint8_t> out(6 + value_.size());
    uint16_t net_type = htons(type_);
    uint32_t net_len = htonl(value_.size());
    memcpy(out.data(), &net_type, 2);
    memcpy(out.data() + 2, &net_len, 4);
    memcpy(out.data() + 6, value_.data(), value_.size());
    return out;
}

std::vector<uint8_t> TLV::serialize_header(uint32_t override_len) const {
    std::vector<uint8_t> out(6);
    uint16_t net_type = htons(type_);
    uint32_t net_len = htonl(override_len);
    memcpy(out.data(), &net_type, 2);
    memcpy(out.data() + 2, &net_len, 4);
    return out;
}
