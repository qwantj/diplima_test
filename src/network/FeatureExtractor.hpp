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

class FeatureExtractor {
public:
    FeatureExtractor();

    // Load scaler params from JSON
    bool loadScalerParams(const std::string& jsonPath);

    // Process a packet within the current window
    void processPacket(pcpp::RawPacket& rawPacket);

    // Finalize the window and compute features for all active flows
    std::vector<std::vector<double>> computeFeaturesBatch();

    // Get normalized features for all active flows (after scaler)
    std::vector<std::vector<double>> computeNormalizedFeaturesBatch();

    // Get normalized features for the heaviest flow (backward compatibility)
    std::vector<double> computeNormalizedFeatures();

    // Get telemetry for the current window
    void fillTelemetry(DetectionResult& result) const;

    // Reset for a new window
    void reset();

    // Set the scaler type for a specific model
    void setModelScaler(const std::string& modelName);

    int featureCount() const { return scaler_.features.empty() ? 8 : (int)scaler_.features.size(); }
    size_t activeFlowsCount() const { return activeFlows_.size(); }

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

    // Per-flow tracking
    struct FlowKey {
        std::string ip1, ip2;
        uint16_t port1, port2;
        uint8_t proto;
        bool operator<(const FlowKey& o) const {
            return std::tie(ip1, ip2, port1, port2, proto) <
                   std::tie(o.ip1, o.ip2, o.port1, o.port2, o.proto);
        }
    };

    struct FlowStats {
        std::string initiatorIp;
        std::chrono::steady_clock::time_point firstPacketTime;
        std::chrono::steady_clock::time_point lastPacketTime;
        uint64_t fwdPackets = 0;
        uint64_t bwdPackets = 0;
        uint64_t fwdBytes = 0;
        uint64_t bwdBytes = 0;
        
        // Extended TCP features
        uint64_t synPackets = 0;
        uint64_t ackPackets = 0;
        uint64_t finPackets = 0;
        uint64_t rstPackets = 0;
        uint64_t pshPackets = 0;
        uint64_t urgPackets = 0;
        uint64_t tcpWindowSizeTotal = 0;
        
        // Payload entropy tracking
        std::vector<uint64_t> byteCounts;
        uint64_t totalPayloadBytes = 0;

        FlowStats() : byteCounts(256, 0) {}
    };

    std::map<FlowKey, FlowStats> activeFlows_;

    // Global Window Counters (for telemetry)
    uint64_t totalPackets_  = 0;
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
