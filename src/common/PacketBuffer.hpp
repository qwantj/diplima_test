#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <RawPacket.h>

struct PacketBuffer {
    static constexpr size_t PREALLOCATED_SIZE = 2048;

    uint8_t staticData[PREALLOCATED_SIZE];
    std::vector<uint8_t> dynamicData;
    size_t length = 0;
    std::chrono::system_clock::time_point timestamp;
    pcpp::LinkLayerType linkType = pcpp::LINKTYPE_ETHERNET;

    void assign(const uint8_t* data, size_t len, const std::chrono::system_clock::time_point& ts, pcpp::LinkLayerType lt = pcpp::LINKTYPE_ETHERNET) {
        length = len;
        timestamp = ts;
        linkType = lt;
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
