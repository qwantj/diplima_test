#include "core/DetectionEngine.hpp"
#include "common/AppLogger.hpp"

#include <chrono>
#include <QDateTime>

DetectionEngine::DetectionEngine() {}

DetectionEngine::~DetectionEngine() { stop(); }

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

    running_ = true;
    inferenceThread_ = std::make_unique<std::thread>(&DetectionEngine::inferenceLoop, this);
    return true;
}

bool DetectionEngine::startReplay(const std::string& pcapPath) {
    if (running_) stop();

    // Replay runs in a separate thread, inference loop processes packets
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
    });

    return true;
}

void DetectionEngine::stop() {
    running_ = false;
    monitor_.stopCapture();

    if (inferenceThread_ && inferenceThread_->joinable())
        inferenceThread_->join();
    inferenceThread_.reset();

    if (dumpEnabled_)
        dumper_.close();
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

void DetectionEngine::inferenceLoop() {
    AppLogger::get()->info("DetectionEngine: inference loop started (window={}s)",
        INFERENCE_WINDOW_SEC);

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

            PacketBuffer* pktBuf = nullptr;
            if (monitor_.dequeuePacketTimed(pktBuf, timeout) && pktBuf != nullptr) {
                // To keep PcapDumper logic compatible for now, wrap it in a dummy RawPacket
                // if dumpEnabled_ is true. Feature extractor logic will be updated soon.

                extractor_.processPacket(pktBuf);

                if (dumpEnabled_) {
                    timespec ts;
                    auto duration = pktBuf->timestamp.time_since_epoch();
                    ts.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
                    ts.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(duration % std::chrono::seconds(1)).count();

                    pcpp::RawPacket rawPkt(pktBuf->data(), pktBuf->size(), ts, false);
                    dumper_.writePacket(rawPkt, 0); // label updated after inference
                }

                monitor_.recycleBuffer(pktBuf);
            }
        }

        if (!running_) break;

        processWindow();
    }

    AppLogger::get()->info("DetectionEngine: inference loop stopped.");
}

void DetectionEngine::processWindow() {
    // Build result and fill telemetry first
    DetectionResult result;
    result.timestamp = QDateTime::currentDateTime();
    result.modelName = inferencer_.modelName();
    extractor_.fillTelemetry(result);

    // Skip inference if window has no packets
    if (result.totalPackets == 0) {
        return;
    }

    // [Step 1] Noise Threshold Check
    // We immediately mark low traffic as Benign without calling the AI model.
    if (result.pps < NOISE_THRESHOLD_PPS) {
        result.label = 0;
        result.confidence = 0.0f;
        result.features = std::vector<double>(8, 0.0);
        
        AppLogger::get()->debug("DetectionEngine: PPS ({:.1f}) below threshold ({:.1f}). Marked as Benign.", 
            result.pps, NOISE_THRESHOLD_PPS);
            
        if (resultCallback_) resultCallback_(result);
        return;
    }

    // [Step 2] Compute and Normalize Features
    auto features = extractor_.computeNormalizedFeatures();
    if (features.empty() || (int)features.size() != extractor_.featureCount())
        return;

    result.features = features;

    // [Step 3] AI Inference
    std::vector<float> featFloat(features.begin(), features.end());
    auto [label, confidence] = inferencer_.predict(featFloat);
    
    result.label = label;
    result.confidence = confidence;

    // Callback
    if (resultCallback_)
        resultCallback_(result);
}
