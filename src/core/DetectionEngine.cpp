#include "core/DetectionEngine.hpp"
#include "common/AppLogger.hpp"
#include "core/FirewallManager.hpp"

#include <chrono>
#include <QDateTime>

#include <cstdlib>

DetectionEngine::DetectionEngine() {}

DetectionEngine::~DetectionEngine() {
    stop();
    FirewallManager::getInstance().clearAllRules();
}

bool DetectionEngine::init(const std::string& modelPath,
                            const std::string& scalerPath,
                            const std::string& ep) {
    if (!inferencer_.loadModel(modelPath, ep))
        return false;

    // Load scaler: try model-specific first, then fallback
    if (!scalerPath.empty()) {
        extractor_.loadScalerParams(scalerPath);
    } else {
        extractor_.setModelScaler(modelPath);
    }

    AppLogger::get()->info("DetectionEngine: initialized with model '{}', EP={}",
        inferencer_.modelName(), ep);
    return true;
}

bool DetectionEngine::startLive(const std::string& interfaceName) {
    if (running_) stop();

    if (!monitor_.startCapture(interfaceName))
        return false;

    isReplayMode_ = false;
    replayFinished_ = false;
    running_ = true;
    inferenceThread_ = std::make_unique<std::thread>(&DetectionEngine::inferenceLoop, this);
    return true;
}

bool DetectionEngine::startReplay(const std::string& pcapPath) {
    if (running_) stop();

    isReplayMode_ = true;
    replayFinished_ = false;
    running_ = true;
    inferenceThread_ = std::make_unique<std::thread>([this, pcapPath]() {
        // Start replay (this blocks until done)
        std::thread replayThread([this, pcapPath]() {
            monitor_.replayPcap(pcapPath);
        });

        // Run inference loop while replay is active
        inferenceLoop();

        if (replayThread.joinable())
            replayThread.join();
        
        AppLogger::get()->info("DetectionEngine: Replay thread joined.");
    });

    return true;
}

void DetectionEngine::stop() {
    running_ = false;
    isReplayMode_ = false;
    monitor_.stopCapture();

    if (inferenceThread_ && inferenceThread_->joinable())
        inferenceThread_->join();
    inferenceThread_.reset();

    if (dumpEnabled_)
        dumper_.close();
}

void DetectionEngine::setMitigationEnabled(bool enabled) {
    isMitigationEnabled_ = enabled;
    if (!enabled) {
        FirewallManager::getInstance().unblockAllIps();
    }
}

void DetectionEngine::setBpfFilter(const std::string& filter) {
    monitor_.setBpfFilter(filter);
}

bool DetectionEngine::hotSwapModel(const std::string& modelPath, const std::string& ep) {
    bool ok = inferencer_.hotSwapModel(modelPath, ep);
    if (ok) {
        extractor_.setModelScaler(modelPath);
    }
    return ok;
}

void DetectionEngine::enablePcapDump(const std::string& dir) {
    pcapDumpDir_ = dir;
    dumpEnabled_ = true;

    auto sessionName = "session_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss").toStdString();
    dumper_.startSession(dir, sessionName);
}

void DetectionEngine::disablePcapDump() {
    dumpEnabled_ = false;
    dumper_.close();
}

void DetectionEngine::inferenceLoop() {
    AppLogger::get()->info("DetectionEngine: inference loop started (window={}s)",
        INFERENCE_WINDOW_SEC);

    // If in replay mode, wait a bit for capturing_ to become true (avoid race)
    if (isReplayMode_) {
        int retries = 0;
        while (!monitor_.isCapturing() && retries++ < 10) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    while (running_) {
        auto windowStart = std::chrono::steady_clock::now();
        extractor_.reset();

        // Collect packets for the window duration
        while (running_) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration<double>(now - windowStart).count();
            if (elapsed >= INFERENCE_WINDOW_SEC)
                break;

            double remaining = INFERENCE_WINDOW_SEC - elapsed;
            auto timeout = std::chrono::microseconds(static_cast<long long>(remaining * 1e6));
            if (timeout.count() < 1000) timeout = std::chrono::microseconds(1000);

            PacketBuffer* pkt = nullptr;
            if (monitor_.dequeuePacket(pkt)) {
                extractor_.processPacket(pkt);
                if (dumpEnabled_) {
                    // Convert back to RawPacket for dumper if needed, or update dumper to take PacketBuffer
                    struct timeval tv = {0, 0};
                    pcpp::RawPacket raw(pkt->data(), pkt->size(), tv, false);
                    dumper_.addPacket(raw);
                }
                monitor_.recycleBuffer(pkt);
            } else {
                // If in replay mode and monitor stopped capturing, and queue is empty, we are done
                if (isReplayMode_ && !monitor_.isCapturing() && monitor_.queueSize() == 0) {
                    replayFinished_ = true;
                    // We don't set running_ = false here to allow processing the last window
                    goto final_window;
                }
                // No packets available, short sleep
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

    final_window:
        if (!running_ && !replayFinished_) break;

        processWindow();
        
        if (replayFinished_) {
            running_ = false;
            break;
        }
    }

    AppLogger::get()->info("DetectionEngine: inference loop stopped.");
}

void DetectionEngine::processWindow() {
    // Build result and fill telemetry first
    DetectionResult result;
    result.timestamp = QDateTime::currentDateTime();
    result.modelName = inferencer_.modelName();
    extractor_.fillTelemetry(result);

    // Fill monitor-specific metrics
    result.droppedPackets = monitor_.droppedPackets();
    result.queueSize = monitor_.queueSize();
    result.blockedIps = FirewallManager::getInstance().getBlockedIps();

    // Skip inference if window has no packets
    if (result.totalPackets == 0) {
        if (dumpEnabled_) dumper_.commitWindow(0);
        return;
    }

    // [Step 1] Noise Threshold Check
    if (result.pps < NOISE_THRESHOLD_PPS) {
        result.label = 0;
        result.confidence = 0.0f;
        result.features = std::vector<double>(extractor_.featureCount(), 0.0);
        
        if (dumpEnabled_) dumper_.commitWindow(0);
        if (resultCallback_) resultCallback_(result);
        return;
    }

    // [Step 2] Compute and Normalize Features (aggregated across all flows)
    auto features = extractor_.computeNormalizedFeatures();
    if (features.empty() || (int)features.size() != extractor_.featureCount())
        return;

    result.features = features;

    // ML Inference
    std::vector<float> featFloat(features.begin(), features.end());

    auto startInfer = std::chrono::steady_clock::now();
    auto [label, confidence] = inferencer_.predict(featFloat);
    auto endInfer = std::chrono::steady_clock::now();

    result.inferenceLatencyMs = std::chrono::duration<double, std::milli>(endInfer - startInfer).count();
    result.label = label;
    result.confidence = confidence;

    // --- Phase 1.5: Commit PCAP Window ---
    if (dumpEnabled_) {
        dumper_.commitWindow(result.label);
    }

    // --- Phase 2: Incident Tracking ---
    updateIncidentState(result);

    // Callback
    if (resultCallback_)
        resultCallback_(result);
}

void DetectionEngine::updateIncidentState(const DetectionResult& result) {
    bool currentlyAttacking = (result.label == 1);

    if (!isUnderAttack_ && currentlyAttacking) {
        // Transition: Normal -> Attack
        isUnderAttack_ = true;
        attackStartTime_ = result.timestamp;
        maxPps_ = result.pps;
        maxConfidence_ = result.confidence;
        attackType_ = "DDoS Attack"; // General label, could be more specific
        attackSrcIps_.clear();
        for (auto& [ip, count] : result.topTalkers) {
            attackSrcIps_[ip] += count;
        }
        AppLogger::get()->info("Incident Tracking: Attack started at {}", 
            attackStartTime_.toString("HH:mm:ss").toStdString());

        if (isMitigationEnabled_ && !result.topTalkers.empty()) {
            FirewallManager::getInstance().blockIp(result.topTalkers[0].first);
        }
    }
    else if (isUnderAttack_ && currentlyAttacking) {
        // Continue: Attack ongoing
        maxPps_ = std::max(maxPps_, result.pps);
        maxConfidence_ = std::max(maxConfidence_, result.confidence);
        for (auto& [ip, count] : result.topTalkers) {
            attackSrcIps_[ip] += count;
        }

        if (isMitigationEnabled_ && !result.topTalkers.empty()) {
            FirewallManager::getInstance().blockIp(result.topTalkers[0].first);
        }
    }
    else if (isUnderAttack_ && !currentlyAttacking) {
        // Transition: Attack -> Normal
        isUnderAttack_ = false;
        float duration = attackStartTime_.secsTo(result.timestamp);
        
        // Determine top attacker IP
        std::string topAttacker = "unknown";
        uint64_t maxPkts = 0;
        for (auto& [ip, count] : attackSrcIps_) {
            if (count > maxPkts) {
                maxPkts = count;
                topAttacker = ip;
            }
        }

        AppLogger::get()->info("Incident Tracking: Attack ended. Duration: {}s, Max PPS: {:.0f}, Top Attacker: {}", 
            duration, maxPps_, topAttacker);

        if (incidentCallback_) {
            incidentCallback_(attackStartTime_, duration, topAttacker, 
                             (float)maxPps_, attackType_, maxConfidence_);
        }
        
        // Unblock all IPs when attack ends (optional, usually WFP rules might expire or be manually managed, 
        // but for this prototype we clear them when the state returns to normal).
        FirewallManager::getInstance().unblockAllIps();
    }
}
