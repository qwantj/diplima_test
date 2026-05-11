#include "network/FeatureExtractor.hpp"
#include "common/AppLogger.hpp"

#include <Packet.h>
#include <EthLayer.h>
#include <IPv4Layer.h>
#include <TcpLayer.h>
#include <UdpLayer.h>
#include <IcmpLayer.h>
#include <PayloadLayer.h>

#include <fstream>
#include <cmath>
#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

FeatureExtractor::FeatureExtractor() {

    sizeHistogram_.resize(5, 0);
    srcIpCounts_.clear();
    dstPortCounts_.clear();
    targetCounts_.clear();
}

bool FeatureExtractor::loadScalerParams(const std::string& jsonPath) {
    try {
        std::ifstream f(jsonPath);
        if (!f.is_open()) {
            AppLogger::get()->error("FeatureExtractor: cannot open scaler: {}", jsonPath);
            return false;
        }
        auto j = nlohmann::json::parse(f);
        scaler_.features = j["features"].get<std::vector<std::string>>();
        scaler_.mean     = j["mean"].get<std::vector<double>>();
        scaler_.scale    = j["scale"].get<std::vector<double>>();
        scaler_.useLog1p = j.value("use_log1p", false);
        scalerLoaded_ = true;
        currentScalerPath_ = jsonPath;
        AppLogger::get()->info("Loaded scaler params from {}", jsonPath);
        return true;
    } catch (const std::exception& e) {
        AppLogger::get()->error("FeatureExtractor: scaler parse error: {}", e.what());
        return false;
    }
}

void FeatureExtractor::processPacket(pcpp::RawPacket& rawPacket) {
    auto now = std::chrono::steady_clock::now();
    if (!windowStarted_) {
        windowStart_ = now;
        windowStarted_ = true;
    }

    pcpp::Packet parsedPacket(&rawPacket);
    int pktLen = rawPacket.getRawDataLen();
    totalBytes_ += pktLen;
    totalPackets_++;

    // Size histogram (5 bins: 0-64, 65-256, 257-512, 513-1024, 1025+)
    if (pktLen <= 64)       sizeHistogram_[0]++;
    else if (pktLen <= 256) sizeHistogram_[1]++;
    else if (pktLen <= 512) sizeHistogram_[2]++;
    else if (pktLen <= 1024)sizeHistogram_[3]++;
    else                    sizeHistogram_[4]++;

    // IP layer
    auto* ipLayer = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();
    std::string srcIp, dstIp;
    if (ipLayer) {
        srcIp = ipLayer->getSrcIPAddress().toString();
        dstIp = ipLayer->getDstIPAddress().toString();
        srcIpCounts_[srcIp]++;
        uniqueSources_.insert(srcIp);
    }

    // Determine ports and protocol for flow key
    uint16_t srcPort = 0, dstPort = 0;
    uint8_t proto = 0;

    bool isTcp = false;
    bool isSyn = false, isAck = false, isFin = false, isRst = false, isPsh = false, isUrg = false;
    uint32_t windowSize = 0;

    if (auto* tcpLayer = parsedPacket.getLayerOfType<pcpp::TcpLayer>()) {
        tcpPackets_++;
        auto* hdr = tcpLayer->getTcpHeader();
        isSyn = hdr->synFlag; if (isSyn) synPackets_++;
        isAck = hdr->ackFlag; 
        isFin = hdr->finFlag; if (isFin) finPackets_++;
        isRst = hdr->rstFlag; if (isRst) rstPackets_++;
        isPsh = hdr->pshFlag; 
        isUrg = hdr->urgFlag; 
        windowSize = ntohs(hdr->windowSize);
        
        srcPort = tcpLayer->getSrcPort();
        dstPort = tcpLayer->getDstPort();
        proto = 6;
        isTcp = true;
        dstPortCounts_[dstPort]++;
        targetCounts_[dstIp + ":" + std::to_string(dstPort)]++;
    }
    else if (auto* udpLayer = parsedPacket.getLayerOfType<pcpp::UdpLayer>()) {
        udpPackets_++;
        srcPort = udpLayer->getSrcPort();
        dstPort = udpLayer->getDstPort();
        proto = 17;
        dstPortCounts_[dstPort]++;
        targetCounts_[dstIp + ":" + std::to_string(dstPort)]++;
    }
    else if (parsedPacket.getLayerOfType<pcpp::IcmpLayer>()) {
        icmpPackets_++;
        proto = 1;
    }
    else {
        otherPackets_++;
    }

    // Flow-based forward/backward classification using 5-tuple
    FlowKey fk;
    if (srcIp < dstIp || (srcIp == dstIp && srcPort <= dstPort)) {
        fk = {srcIp, dstIp, srcPort, dstPort, proto};
    } else {
        fk = {dstIp, srcIp, dstPort, srcPort, proto};
    }

    auto& stats = activeFlows_[fk];
    if (stats.initiatorIp.empty()) {
        stats.initiatorIp = srcIp;
        stats.firstPacketTime = now;
    }
    stats.lastPacketTime = now;

    bool isForward = (stats.initiatorIp == srcIp);

    if (isForward) {
        stats.fwdPackets++;
        stats.fwdBytes += pktLen;
    } else {
        stats.bwdPackets++;
        stats.bwdBytes += pktLen;
    }

    if (isTcp) {
        if (isSyn) stats.synPackets++;
        if (isAck) stats.ackPackets++;
        if (isFin) stats.finPackets++;
        if (isRst) stats.rstPackets++;
        if (isPsh) stats.pshPackets++;
        if (isUrg) stats.urgPackets++;
        stats.tcpWindowSizeTotal += windowSize;
    }

    // Payload Entropy
    if (auto* payload = parsedPacket.getLayerOfType<pcpp::PayloadLayer>()) {
        const uint8_t* payloadData = payload->getPayload();
        size_t payloadLen = payload->getPayloadLen();
        stats.totalPayloadBytes += payloadLen;
        for (size_t i = 0; i < payloadLen; ++i) {
            stats.byteCounts[payloadData[i]]++;
        }
    }
}

std::vector<std::vector<double>> FeatureExtractor::computeFeaturesBatch() {
    std::vector<std::vector<double>> batch;
    batch.reserve(activeFlows_.size());

    for (const auto& [key, stats] : activeFlows_) {
        double flowDuration = std::chrono::duration<double>(stats.lastPacketTime - stats.firstPacketTime).count() * 1e6;
        if (flowDuration <= 0.0) flowDuration = 1.0; // avoid div by zero
        
        double totalFwdPackets = (double)stats.fwdPackets;
        double totalBwdPackets = (double)stats.bwdPackets;
        double totalLenFwd = (double)stats.fwdBytes;
        double totalLenBwd = (double)stats.bwdBytes;
        double fwdMean = (stats.fwdPackets > 0) ? (double)stats.fwdBytes / stats.fwdPackets : 0.0;
        double bwdMean = (stats.bwdPackets > 0) ? (double)stats.bwdBytes / stats.bwdPackets : 0.0;
        double flowPps = (stats.fwdPackets + stats.bwdPackets) / (flowDuration / 1e6);
        
        double entropy = 0.0;
        if (stats.totalPayloadBytes > 0) {
            for (uint64_t count : stats.byteCounts) {
                if (count > 0) {
                    double p = (double)count / stats.totalPayloadBytes;
                    entropy -= p * std::log2(p);
                }
            }
        }

        double avgWindowSize = (stats.fwdPackets + stats.bwdPackets > 0) ? 
            (double)stats.tcpWindowSizeTotal / (stats.fwdPackets + stats.bwdPackets) : 0.0;

        // Base 8 features
        std::vector<double> features = {
            flowDuration, totalFwdPackets, totalBwdPackets, totalLenFwd, totalLenBwd, fwdMean, bwdMean, flowPps
        };
        
        // Extended features (if mapped by scaler, they'll be used, else ignored by model)
        features.push_back((double)stats.synPackets);
        features.push_back((double)stats.ackPackets);
        features.push_back((double)stats.finPackets);
        features.push_back((double)stats.rstPackets);
        features.push_back((double)stats.pshPackets);
        features.push_back((double)stats.urgPackets);
        features.push_back(entropy);
        features.push_back(avgWindowSize);

        batch.push_back(features);
    }
    return batch;
}

std::vector<std::vector<double>> FeatureExtractor::computeNormalizedFeaturesBatch() {
    auto rawBatch = computeFeaturesBatch();
    if (!scalerLoaded_) return rawBatch;

    std::vector<std::vector<double>> normBatch;
    normBatch.reserve(rawBatch.size());

    size_t expectedFeatures = scaler_.mean.size();

    for (const auto& raw : rawBatch) {
        std::vector<double> norm(expectedFeatures, 0.0);
        for (size_t i = 0; i < expectedFeatures && i < raw.size(); i++) {
            double x = raw[i];
            if (scaler_.useLog1p) x = std::log1p(std::max(0.0, x));
            norm[i] = (scaler_.scale[i] != 0.0) ? (x - scaler_.mean[i]) / scaler_.scale[i] : 0.0;
        }
        normBatch.push_back(norm);
    }
    return normBatch;
}

std::vector<double> FeatureExtractor::computeNormalizedFeatures() {
    auto batch = computeNormalizedFeaturesBatch();
    if (batch.empty()) {
        return std::vector<double>(scalerLoaded_ ? scaler_.mean.size() : 8, 0.0);
    }
    
    // For single-vector output (to not break DetectionEngine), return the features
    // of the flow with the highest packet count (Top Talker Flow).
    size_t topFlowIdx = 0;
    uint64_t maxPackets = 0;
    
    int idx = 0;
    for (const auto& [key, stats] : activeFlows_) {
        uint64_t pkts = stats.fwdPackets + stats.bwdPackets;
        if (pkts > maxPackets) {
            maxPackets = pkts;
            topFlowIdx = idx;
        }
        idx++;
    }
    
    return batch[topFlowIdx];
}

void FeatureExtractor::fillTelemetry(DetectionResult& result) const {
    auto now = std::chrono::steady_clock::now();
    double deltaT = windowStarted_ ?
        std::chrono::duration<double>(now - windowStart_).count() : 2.0;
    if (deltaT < 0.001) deltaT = 0.001;

    result.totalPackets = totalPackets_;
    result.tcpPackets   = tcpPackets_;
    result.udpPackets   = udpPackets_;
    result.icmpPackets  = icmpPackets_;
    result.otherPackets = otherPackets_;
    result.synPackets   = synPackets_;
    result.finPackets   = finPackets_;
    result.rstPackets   = rstPackets_;
    result.totalBytes   = totalBytes_;
    result.flowDuration = deltaT;
    result.pps = totalPackets_ / deltaT;
    result.uniqueSourceCount = (uint32_t)uniqueSources_.size();
    result.packetSizeHistogram = sizeHistogram_;
    result.activeFlowsCount = (uint32_t)activeFlows_.size();

    double total = (double)result.totalPackets;
    if (total > 0) {
        result.protocolBreakdown["tcp"]   = tcpPackets_ / total * 100.0;
        result.protocolBreakdown["udp"]   = udpPackets_ / total * 100.0;
        result.protocolBreakdown["icmp"]  = icmpPackets_ / total * 100.0;
        result.protocolBreakdown["other"] = otherPackets_ / total * 100.0;
    }

    std::vector<std::pair<std::string, uint64_t>> talkers(srcIpCounts_.begin(), srcIpCounts_.end());
    std::sort(talkers.begin(), talkers.end(),
        [](auto& a, auto& b) { return a.second > b.second; });
    result.topTalkers.assign(talkers.begin(), talkers.begin() + std::min(talkers.size(), (size_t)10));

    std::vector<std::pair<uint16_t, uint64_t>> ports(dstPortCounts_.begin(), dstPortCounts_.end());
    std::sort(ports.begin(), ports.end(),
        [](auto& a, auto& b) { return a.second > b.second; });
    result.topPorts.assign(ports.begin(), ports.begin() + std::min(ports.size(), (size_t)10));

    std::vector<std::pair<std::string, uint64_t>> targets(targetCounts_.begin(), targetCounts_.end());
    std::sort(targets.begin(), targets.end(),
        [](auto& a, auto& b) { return a.second > b.second; });
    result.topTargets.assign(targets.begin(), targets.begin() + std::min(targets.size(), (size_t)10));
}

void FeatureExtractor::reset() {
    windowStarted_ = false;
    activeFlows_.clear();
    totalPackets_ = tcpPackets_ = udpPackets_ = icmpPackets_ = otherPackets_ = 0;
    synPackets_ = finPackets_ = rstPackets_ = 0;
    totalBytes_ = 0;
    srcIpCounts_.clear();
    dstPortCounts_.clear();
    targetCounts_.clear();
    uniqueSources_.clear();
    sizeHistogram_.assign(5, 0);


    // Also reset window start time for safety
    windowStart_ = std::chrono::steady_clock::now();
}

void FeatureExtractor::setModelScaler(const std::string& modelName) {
    // Try model-specific scaler first: e.g. "mlp_model.onnx" -> "mlp_scaler_params.json"
    namespace fs = std::filesystem;
    fs::path p(modelName);

    fs::path dir = p.parent_path();
    if (dir.empty()) {
        dir = "models";
    }

    std::string stem = p.stem().string();
    // Remove "_model" suffix if present
    auto mpos = stem.find("_model");
    if (mpos != std::string::npos) {
        stem = stem.substr(0, mpos);
    }

    std::string specific = (dir / (stem + "_scaler_params.json")).string();
    std::string fallback = (dir / "scaler_params.json").string();

    if (!loadScalerParams(specific)) {
        loadScalerParams(fallback);
    }
}
