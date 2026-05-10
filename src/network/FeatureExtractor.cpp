#include "network/FeatureExtractor.hpp"
#include "common/AppLogger.hpp"

#include <Packet.h>
#include <EthLayer.h>
#include <IPv4Layer.h>
#include <TcpLayer.h>
#include <UdpLayer.h>
#include <IcmpLayer.h>

#include <fstream>
#include <cmath>
#include <algorithm>

FeatureExtractor::FeatureExtractor() {
    sizeHistogram_.resize(5, 0);
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
    if (!windowStarted_) {
        windowStart_ = std::chrono::steady_clock::now();
        windowStarted_ = true;
    }

    pcpp::Packet parsedPacket(&rawPacket);
    int pktLen = rawPacket.getRawDataLen();
    totalBytes_ += pktLen;

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

    if (auto* tcpLayer = parsedPacket.getLayerOfType<pcpp::TcpLayer>()) {
        tcpPackets_++;
        auto* hdr = tcpLayer->getTcpHeader();
        if (hdr->synFlag) synPackets_++;
        if (hdr->finFlag) finPackets_++;
        if (hdr->rstFlag) rstPackets_++;
        srcPort = tcpLayer->getSrcPort();
        dstPort = tcpLayer->getDstPort();
        proto = 6;
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
    // Create canonical flow key (sorted IPs for direction-independent matching)
    FlowKey fk;
    if (srcIp < dstIp || (srcIp == dstIp && srcPort <= dstPort)) {
        fk = {srcIp, dstIp, srcPort, dstPort, proto};
    } else {
        fk = {dstIp, srcIp, dstPort, srcPort, proto};
    }

    auto it = flowInitiators_.find(fk);
    bool isForward;
    if (it == flowInitiators_.end()) {
        // New flow — this src is the initiator (forward direction)
        flowInitiators_[fk] = srcIp;
        isForward = true;
    } else {
        // Known flow — check if src is the initiator
        isForward = (it->second == srcIp);
    }

    if (isForward) {
        fwdPackets_++;
        fwdBytes_ += pktLen;
    } else {
        bwdPackets_++;
        bwdBytes_ += pktLen;
    }
}

std::vector<double> FeatureExtractor::computeFeatures() {
    auto now = std::chrono::steady_clock::now();
    double deltaT = 0.0;
    if (windowStarted_) {
        deltaT = std::chrono::duration<double>(now - windowStart_).count();
        if (deltaT < 0.001) deltaT = 0.001;
    } else {
        deltaT = 2.0; // default window
    }

    // 8 features in CICIDS order
    double flowDuration        = deltaT * 1e6; // microseconds
    double totalFwdPackets     = (double)fwdPackets_;
    double totalBwdPackets     = (double)bwdPackets_;
    double totalLenFwd         = (double)fwdBytes_;
    double totalLenBwd         = (double)bwdBytes_;
    double fwdMean             = (fwdPackets_ > 0) ? (double)fwdBytes_ / fwdPackets_ : 0.0;
    double bwdMean             = (bwdPackets_ > 0) ? (double)bwdBytes_ / bwdPackets_ : 0.0;
    double flowPps             = (fwdPackets_ + bwdPackets_) / deltaT;

    return {flowDuration, totalFwdPackets, totalBwdPackets,
            totalLenFwd, totalLenBwd, fwdMean, bwdMean, flowPps};
}

std::vector<double> FeatureExtractor::computeNormalizedFeatures() {
    auto raw = computeFeatures();
    if (!scalerLoaded_ || scaler_.mean.size() != raw.size())
        return raw;

    std::vector<double> norm(raw.size());
    for (size_t i = 0; i < raw.size(); i++) {
        double x = raw[i];
        if (scaler_.useLog1p)
            x = std::log1p(std::max(0.0, x));
        norm[i] = (scaler_.scale[i] != 0.0) ? (x - scaler_.mean[i]) / scaler_.scale[i] : 0.0;
    }
    return norm;
}

void FeatureExtractor::fillTelemetry(DetectionResult& result) const {
    auto now = std::chrono::steady_clock::now();
    double deltaT = windowStarted_ ?
        std::chrono::duration<double>(now - windowStart_).count() : 2.0;
    if (deltaT < 0.001) deltaT = 0.001;

    result.totalPackets = fwdPackets_ + bwdPackets_;
    result.tcpPackets   = tcpPackets_;
    result.udpPackets   = udpPackets_;
    result.icmpPackets  = icmpPackets_;
    result.otherPackets = otherPackets_;
    result.synPackets   = synPackets_;
    result.finPackets   = finPackets_;
    result.rstPackets   = rstPackets_;
    result.totalBytes   = totalBytes_;
    result.flowDuration = deltaT;
    result.pps = result.totalPackets / deltaT;
    result.uniqueSourceCount = (uint32_t)uniqueSources_.size();
    result.packetSizeHistogram = sizeHistogram_;
    // Drop rate will be filled by DetectionEngine as it has access to monitor

    // Protocol breakdown (percentages)
    double total = (double)result.totalPackets;
    if (total > 0) {
        result.protocolBreakdown["tcp"]   = tcpPackets_ / total * 100.0;
        result.protocolBreakdown["udp"]   = udpPackets_ / total * 100.0;
        result.protocolBreakdown["icmp"]  = icmpPackets_ / total * 100.0;
        result.protocolBreakdown["other"] = otherPackets_ / total * 100.0;
    }

    // Top talkers (source IPs)
    std::vector<std::pair<std::string, uint64_t>> talkers(srcIpCounts_.begin(), srcIpCounts_.end());
    std::sort(talkers.begin(), talkers.end(),
        [](auto& a, auto& b) { return a.second > b.second; });
    result.topTalkers.assign(talkers.begin(),
        talkers.begin() + std::min(talkers.size(), (size_t)10));

    // Top ports
    std::vector<std::pair<uint16_t, uint64_t>> ports(dstPortCounts_.begin(), dstPortCounts_.end());
    std::sort(ports.begin(), ports.end(),
        [](auto& a, auto& b) { return a.second > b.second; });
    result.topPorts.assign(ports.begin(),
        ports.begin() + std::min(ports.size(), (size_t)10));

    // Top targets
    std::vector<std::pair<std::string, uint64_t>> targets(targetCounts_.begin(), targetCounts_.end());
    std::sort(targets.begin(), targets.end(),
        [](auto& a, auto& b) { return a.second > b.second; });
    result.topTargets.assign(targets.begin(),
        targets.begin() + std::min(targets.size(), (size_t)10));
}

void FeatureExtractor::reset() {
    windowStarted_ = false;
    fwdPackets_ = bwdPackets_ = 0;
    fwdBytes_ = bwdBytes_ = 0;
    tcpPackets_ = udpPackets_ = icmpPackets_ = otherPackets_ = 0;
    synPackets_ = finPackets_ = rstPackets_ = 0;
    totalBytes_ = 0;
    srcIpCounts_.clear();
    dstPortCounts_.clear();
    targetCounts_.clear();
    uniqueSources_.clear();
    sizeHistogram_.assign(5, 0);
    flowInitiators_.clear();
}

void FeatureExtractor::setModelScaler(const std::string& modelName) {
    // Try model-specific scaler first: e.g. "mlp_model.onnx" -> "mlp_scaler_params.json"
    std::string base = modelName;
    auto pos = base.rfind('.');
    if (pos != std::string::npos) base = base.substr(0, pos);
    // Try e.g. "models/mlp_scaler_params.json"
    pos = base.rfind('/');
    if (pos == std::string::npos) pos = base.rfind('\\');
    std::string dir = (pos != std::string::npos) ? base.substr(0, pos + 1) : "models/";
    std::string name = (pos != std::string::npos) ? base.substr(pos + 1) : base;

    // Remove "_model" suffix if present
    auto mpos = name.find("_model");
    if (mpos != std::string::npos) name = name.substr(0, mpos);

    std::string specific = dir + name + "_scaler_params.json";
    std::string fallback = dir + "scaler_params.json";

    if (!loadScalerParams(specific)) {
        loadScalerParams(fallback);
    }
}
