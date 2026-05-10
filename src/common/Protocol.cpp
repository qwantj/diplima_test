#include "common/Protocol.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDataStream>

namespace Protocol {

nlohmann::json serializeResult(const DetectionResult& r) {
    nlohmann::json j;
    j["version"]     = PROTOCOL_VERSION;
    j["label"]       = r.label;
    j["confidence"]  = r.confidence;
    j["pps"]         = r.pps;
    j["total_packets"]= r.totalPackets;
    j["tcp"]         = r.tcpPackets;
    j["udp"]         = r.udpPackets;
    j["icmp"]        = r.icmpPackets;
    j["other"]       = r.otherPackets;
    j["syn"]         = r.synPackets;
    j["fin"]         = r.finPackets;
    j["rst"]         = r.rstPackets;
    j["total_bytes"] = r.totalBytes;
    j["flow_duration"]= r.flowDuration;
    j["dropped_packets"] = r.droppedPackets;
    j["queue_size"] = r.queueSize;
    j["inference_latency_ms"] = r.inferenceLatencyMs;
    j["features"]    = r.features;
    j["model"]       = r.modelName;
    j["session_id"]  = r.sessionId;
    j["timestamp"]   = r.timestamp.toString(Qt::ISODateWithMs).toStdString();
    j["unique_sources"] = r.uniqueSourceCount;

    if (!r.protocolBreakdown.empty())
        j["protocol_breakdown"] = r.protocolBreakdown;
    if (!r.packetSizeHistogram.empty())
        j["packet_size_histogram"] = r.packetSizeHistogram;

    nlohmann::json talkers = nlohmann::json::array();
    for (auto& [ip, cnt] : r.topTalkers)
        talkers.push_back({{"ip", ip}, {"count", cnt}});
    if (!talkers.empty()) j["top_talkers"] = talkers;

    nlohmann::json ports = nlohmann::json::array();
    for (auto& [port, cnt] : r.topPorts)
        ports.push_back({{"port", port}, {"count", cnt}});
    if (!ports.empty()) j["top_ports"] = ports;

    nlohmann::json targets = nlohmann::json::array();
    for (auto& [t, cnt] : r.topTargets)
        targets.push_back({{"target", t}, {"count", cnt}});
    if (!targets.empty()) j["top_targets"] = targets;

    return j;
}

DetectionResult deserializeResult(const nlohmann::json& j) {
    DetectionResult r;
    r.label        = j.value("label", 0);
    r.confidence   = j.value("confidence", 0.0f);
    r.pps          = j.value("pps", 0.0);
    r.totalPackets = j.value("total_packets", (uint64_t)0);
    r.tcpPackets   = j.value("tcp", (uint64_t)0);
    r.udpPackets   = j.value("udp", (uint64_t)0);
    r.icmpPackets  = j.value("icmp", (uint64_t)0);
    r.otherPackets = j.value("other", (uint64_t)0);
    r.synPackets   = j.value("syn", (uint64_t)0);
    r.finPackets   = j.value("fin", (uint64_t)0);
    r.rstPackets   = j.value("rst", (uint64_t)0);
    r.totalBytes   = j.value("total_bytes", (uint64_t)0);
    r.flowDuration = j.value("flow_duration", 0.0);
    r.droppedPackets = j.value("dropped_packets", (uint64_t)0);
    r.queueSize    = j.value("queue_size", (size_t)0);
    r.inferenceLatencyMs = j.value("inference_latency_ms", 0.0);
    r.modelName    = j.value("model", std::string());
    r.sessionId    = j.value("session_id", 0);
    r.uniqueSourceCount = j.value("unique_sources", (uint32_t)0);

    if (j.contains("features"))
        r.features = j["features"].get<std::vector<double>>();
    if (j.contains("timestamp"))
        r.timestamp = QDateTime::fromString(
            QString::fromStdString(j["timestamp"].get<std::string>()), Qt::ISODateWithMs);

    if (j.contains("protocol_breakdown"))
        r.protocolBreakdown = j["protocol_breakdown"].get<std::map<std::string, double>>();
    if (j.contains("packet_size_histogram"))
        r.packetSizeHistogram = j["packet_size_histogram"].get<std::vector<int>>();

    if (j.contains("top_talkers"))
        for (auto& t : j["top_talkers"])
            r.topTalkers.emplace_back(t["ip"].get<std::string>(), t["count"].get<uint64_t>());
    if (j.contains("top_ports"))
        for (auto& p : j["top_ports"])
            r.topPorts.emplace_back(p["port"].get<uint16_t>(), p["count"].get<uint64_t>());
    if (j.contains("top_targets"))
        for (auto& t : j["top_targets"])
            r.topTargets.emplace_back(t["target"].get<std::string>(), t["count"].get<uint64_t>());

    return r;
}

QByteArray serializeSnapshot(float pps, uint64_t totalPackets, int currentLabel) {
    nlohmann::json j;
    j["version"]       = PROTOCOL_VERSION;
    j["type"]          = MSG_SNAPSHOT;
    j["pps"]           = pps;
    j["total_packets"] = totalPackets;
    j["current_label"] = currentLabel;
    return frame(QByteArray::fromStdString(j.dump()));
}

QByteArray serializeStats(const DetectionResult& r, uint64_t totalPackets) {
    nlohmann::json j;
    j["version"]       = PROTOCOL_VERSION;
    j["type"]          = MSG_STATS;
    j["total_packets"] = totalPackets;
    j["data"]          = serializeResult(r);
    return frame(QByteArray::fromStdString(j.dump()));
}

QByteArray serializeCommand(const std::string& cmd, const nlohmann::json& data) {
    nlohmann::json j;
    j["version"] = PROTOCOL_VERSION;
    j["type"] = MSG_CMD;
    j["cmd"]  = cmd;
    if (!data.is_null()) j["data"] = data;
    return frame(QByteArray::fromStdString(j.dump()));
}

QByteArray serializeNotify(const std::string& event, const nlohmann::json& data) {
    nlohmann::json j;
    j["version"] = PROTOCOL_VERSION;
    j["type"]  = MSG_NOTIFY;
    j["event"] = event;
    if (!data.is_null()) j["data"] = data;
    return frame(QByteArray::fromStdString(j.dump()));
}

std::pair<std::string, nlohmann::json> parseMessage(const QByteArray& raw) {
    try {
        auto j = nlohmann::json::parse(raw.toStdString());
        return {j.value("type", ""), j};
    } catch (...) {
        return {"error", {}};
    }
}

QByteArray frame(const QByteArray& payload) {
    return payload + "\n";
}

} // namespace Protocol
