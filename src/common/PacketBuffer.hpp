#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <chrono>

struct PacketBuffer {
    static constexpr size_t PREALLOCATED_SIZE = 2048;

    uint8_t staticData[PREALLOCATED_SIZE];
    std::vector<uint8_t> dynamicData;
    size_t length = 0;
    std::chrono::system_clock::time_point timestamp;

    void assign(const uint8_t* data, size_t len, const std::chrono::system_clock::time_point& ts) {
        length = len;
        timestamp = ts;
        if (len <= PREALLOCATED_SIZE) {
            std::memcpy(staticData, data, len);
        } else {
            dynamicData.assign(data, data + len);
        }
    }

    const uint8_t* data() const {
        return length <= PREALLOCATED_SIZE ? staticData : dynamicData.data();
    }

    size_t size() const {
        return length;
    }
};
