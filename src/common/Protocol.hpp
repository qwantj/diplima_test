#pragma once

#include <nlohmann/json.hpp>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <cstdint>
#include <string>
#include <vector>
#include <map>

// ---- Core data structures (Global Namespace for compatibility) ----

struct DetectionResult {
    QDateTime   timestamp;
    int         label = 0; // 0: Benign, 1: Attack
    float       confidence = 0.0f;
    double      pps = 0.0;
    uint64_t    totalPackets = 0;
    uint64_t    tcpPackets   = 0;
    uint64_t    udpPackets   = 0;
    uint64_t    icmpPackets  = 0;
    uint64_t    otherPackets = 0;
    uint64_t    synPackets   = 0;
    uint64_t    finPackets   = 0;
    uint64_t    rstPackets   = 0;
    uint64_t    totalBytes   = 0;
    double      flowDuration = 0.0;
    uint64_t    droppedPackets = 0;
    size_t      queueSize    = 0;
    double      inferenceLatencyMs = 0.0;

    // CICIDS features (8)
    std::vector<double> features;

    // Telemetry extras
    std::map<std::string, double> protocolBreakdown;
    std::vector<int>              packetSizeHistogram; // 5 bins
    std::vector<std::pair<std::string, uint64_t>> topTalkers;
    std::vector<std::pair<uint16_t, uint64_t>>    topPorts;
    std::vector<std::pair<std::string, uint64_t>> topTargets;
    uint32_t uniqueSourceCount = 0;
    uint32_t activeFlowsCount = 0;
    std::vector<std::string> blockedIps;

    int         sessionId = 0;
    std::string modelName;
};

struct SessionInfo {
    int       id = 0;
    QString   interfaceName;
    QString   modelName;
    QDateTime startTime;
    QDateTime endTime;
    uint64_t  totalPackets = 0;
    uint64_t  totalAttacks = 0;
    uint64_t  totalBenign  = 0;
};

// ---- Protocol helpers ----

namespace Protocol {

const std::string PROTOCOL_VERSION = "2.2";

// Message types
constexpr const char* MSG_STATS       = "stats";
constexpr const char* MSG_SNAPSHOT    = "snapshot";
constexpr const char* MSG_CMD         = "cmd";
constexpr const char* MSG_NOTIFY      = "notify";
constexpr const char* MSG_BUFFER      = "buffer";

// Commands
constexpr const char* CMD_LOAD_PCAP   = "load_pcap";
constexpr const char* CMD_STOP_REPLAY = "stop_replay";
constexpr const char* CMD_LOAD_MODEL  = "load_model";
constexpr const char* CMD_CONFIG_BPF  = "config_bpf";
constexpr const char* CMD_CONFIG_DUMP = "config_dump";
constexpr const char* CMD_CONFIG_UPDATE = "config_update";
constexpr const char* CMD_STOP        = "stop";

// Serialize DetectionResult to JSON
nlohmann::json serializeResult(const DetectionResult& r);

// Deserialize DetectionResult from JSON
DetectionResult deserializeResult(const nlohmann::json& j);

// Serialize a snapshot message
QByteArray serializeSnapshot(float pps, uint64_t totalPackets, int currentLabel);

// Serialize full stats message
QByteArray serializeStats(const DetectionResult& r, uint64_t totalPackets);

// Serialize a command
QByteArray serializeCommand(const std::string& cmd, const nlohmann::json& data = {});

// Serialize a notification
QByteArray serializeNotify(const std::string& event, const nlohmann::json& data = {});

// Parse an incoming message and return (type, payload)
std::pair<std::string, nlohmann::json> parseMessage(const QByteArray& raw);

// Frame/unframe: length-prefixed
QByteArray frame(const QByteArray& payload);

} // namespace Protocol

Q_DECLARE_METATYPE(DetectionResult)
Q_DECLARE_METATYPE(SessionInfo)
Q_DECLARE_METATYPE(std::vector<DetectionResult>)
Q_DECLARE_METATYPE(std::vector<SessionInfo>)
Q_DECLARE_METATYPE(std::vector<QByteArray>)
