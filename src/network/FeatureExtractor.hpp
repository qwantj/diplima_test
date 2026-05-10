#pragma once

#include <vector>
#include <map>
#include <set>
#include <string>
#include <cstdint>
#include <chrono>
#include <tuple>

#include <RawPacket.h>
#include <nlohmann/json.hpp>

#include "common/Protocol.hpp"
#include "common/PacketBuffer.hpp"

class FeatureExtractor {
public:
    FeatureExtractor();

    // Load scaler params from JSON
    bool loadScalerParams(const std::string& jsonPath);

    // Process a packet within the current window
    void processPacket(PacketBuffer* pktBuf);
    void processPacket(pcpp::RawPacket& rawPacket); // deprecated/fallback

    // Finalize the window and compute features
    std::vector<double> computeFeatures();

    // Get normalized features (after scaler)
    std::vector<double> computeNormalizedFeatures();

    // Get telemetry for the current window
    void fillTelemetry(DetectionResult& result) const;

    // Reset for a new window
    void reset();

    // Set the scaler type for a specific model
    void setModelScaler(const std::string& modelName);

    int featureCount() const { return 8; }

private:
    struct ScalerParams {
        std::vector<std::string> features;
        std::vector<double> mean;
        std::vector<double> scale;
        bool useLog1p = false;
    };

    ScalerParams scaler_;
    bool scalerLoaded_ = false;
    std::string currentScalerPath_;

    // Window state
    std::chrono::steady_clock::time_point windowStart_;
    bool windowStarted_ = false;

    // Per-flow tracking for fwd/bwd classification
    // Key: canonical 5-tuple (sorted IPs)
    // Value: initiator direction (src_ip of first packet in flow)
    struct FlowKey {
        std::string ip1, ip2;
        uint16_t port1, port2;
        uint8_t proto;
        bool operator<(const FlowKey& o) const {
            return std::tie(ip1, ip2, port1, port2, proto) <
                   std::tie(o.ip1, o.ip2, o.port1, o.port2, o.proto);
        }
    };
    std::map<FlowKey, std::string> flowInitiators_; // flowKey -> initiator srcIp

    // Counters
    uint64_t fwdPackets_    = 0;
    uint64_t bwdPackets_    = 0;
    uint64_t fwdBytes_      = 0;
    uint64_t bwdBytes_      = 0;
    uint64_t tcpPackets_    = 0;
    uint64_t udpPackets_    = 0;
    uint64_t icmpPackets_   = 0;
    uint64_t otherPackets_  = 0;
    uint64_t synPackets_    = 0;
    uint64_t finPackets_    = 0;
    uint64_t rstPackets_    = 0;
    uint64_t totalBytes_    = 0;

    // Telemetry
    std::map<std::string, uint64_t> srcIpCounts_;
    std::map<uint16_t, uint64_t>    dstPortCounts_;
    std::map<std::string, uint64_t> targetCounts_;
    std::set<std::string>           uniqueSources_;
    std::vector<int>                sizeHistogram_; // 5 bins
};
