#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <memory>
#include <deque>

#include "network/TrafficMonitor.hpp"
#include "network/FeatureExtractor.hpp"
#include "network/PcapDumper.hpp"
#include "ml/ModelInferencer.hpp"
#include "common/Protocol.hpp"

#include <pcapplusplus/RawPacket.h>

class DetectionEngine {
public:
    DetectionEngine();
    ~DetectionEngine();

    // Callbacks
    using ResultCallback = std::function<void(const DetectionResult&)>;
    using IncidentCallback = std::function<void(const QDateTime& startTime, float duration, const std::string& attackerIp, float ppsMax, const std::string& typeLabel, float confidence)>;
    
    void setResultCallback(ResultCallback cb) { resultCallback_ = cb; }
    void setIncidentCallback(IncidentCallback cb) { incidentCallback_ = cb; }

    // Initialize
    bool init(const std::string& modelPath,
              const std::string& scalerPath = "",
              const std::string& ep = "cpu");

    // Start live detection
    bool startLive(const std::string& interfaceName);

    // Start replay detection
    bool startReplay(const std::string& pcapPath);

    void stop();
    bool isRunning() const { return running_; }

    // BPF filter
    void setBpfFilter(const std::string& filter);

    // Hot-swap model
    bool hotSwapModel(const std::string& modelPath, const std::string& ep = "cpu");

    // Pcap dumper
    void enablePcapDump(const std::string& dir);

    // Active Mitigation
    void setMitigationEnabled(bool enabled);

    // Access submodules
    TrafficMonitor& trafficMonitor() { return monitor_; }
    FeatureExtractor& featureExtractor() { return extractor_; }
    ModelInferencer& modelInferencer() { return inferencer_; }

    static constexpr double INFERENCE_WINDOW_SEC = 2.0;

private:
    void inferenceLoop();
    void processWindow();
    void updateIncidentState(const DetectionResult& result);
    void blockIp(const std::string& ip);
    void unblockAllIps();

    TrafficMonitor   monitor_;
    FeatureExtractor extractor_;
    ModelInferencer  inferencer_;
    PcapDumper       dumper_;

    ResultCallback resultCallback_;
    IncidentCallback incidentCallback_;
    std::atomic<bool> running_{false};
    std::atomic<bool> isReplayMode_{false};
    std::atomic<bool> replayFinished_{false};
    std::unique_ptr<std::thread> inferenceThread_;

    std::string pcapDumpDir_;
    bool dumpEnabled_ = false;

    // False Positive Mitigation
    static constexpr double NOISE_THRESHOLD_PPS = 50.0; // Higher threshold for guaranteed silence

    // Incident tracking
    bool isUnderAttack_ = false;
    bool isMitigationEnabled_ = false;
    QDateTime attackStartTime_;
    double maxPps_ = 0.0;
    std::string attackType_;
    float maxConfidence_ = 0.0;
    std::map<std::string, uint64_t> attackSrcIps_;
    std::set<std::string> activeBlockedIps_;
};
