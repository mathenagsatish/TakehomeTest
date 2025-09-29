#pragma once
#include <cstdint>
#include <string>
#include <vector>

class TLV {
public:
    TLV(uint16_t type, const std::string& value);
    std::vector<uint8_t> serialize() const;
    std::vector<uint8_t> serialize_header(uint32_t override_len) const;
private:
    uint16_t type_;
    std::string value_;
};
