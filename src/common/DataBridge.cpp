#include "common/DataBridge.hpp"
#include "common/AppLogger.hpp"

DataBridge::DataBridge(QObject* parent)
    : QObject(parent)
    , tcpClient_(new TcpClient(this))
    , dbManager_(new DatabaseManager(this))
{
    connect(tcpClient_, &TcpClient::connectedToServer,
            this, &DataBridge::onCollectorConnected);
    connect(tcpClient_, &TcpClient::disconnectedFromServer,
            this, &DataBridge::onCollectorDisconnected);
    connect(tcpClient_, &TcpClient::statsReceived,
            this, &DataBridge::realtimeStatsReceived);
    connect(tcpClient_, &TcpClient::snapshotReceived,
            this, &DataBridge::realtimeSnapshotReceived);
    connect(tcpClient_, &TcpClient::bufferReceived,
            this, &DataBridge::onHistoricalBufferReceived);
    connect(tcpClient_, &TcpClient::notificationReceived,
            this, &DataBridge::notificationReceived);
}

DataBridge::~DataBridge() = default;

void DataBridge::connectToCollector(const QString& host, quint16 port) {
    tcpClient_->connectToCollector(host, port);
}

void DataBridge::connectToDatabase(const QString& host, int port,
                                    const QString& dbName,
                                    const QString& user,
                                    const QString& password) {
    bool ok = dbManager_->connectToDatabase(host, port, dbName, user, password);
    emit databaseConnectionChanged(ok);
}

void DataBridge::sendBpfConfig(bool enable) {
    nlohmann::json data; data["enable"] = enable;
    tcpClient_->sendCommand(Protocol::CMD_CONFIG_BPF, data);
}

void DataBridge::sendDumpConfig(bool enable) {
    nlohmann::json data; data["enable"] = enable;
    tcpClient_->sendCommand(Protocol::CMD_CONFIG_DUMP, data);
}

void DataBridge::onCollectorConnected() {
    emit collectorConnectionChanged(true);
}

void DataBridge::onCollectorDisconnected() {
    emit collectorConnectionChanged(false);
}

void DataBridge::onHistoricalBufferReceived(const std::vector<QByteArray>& rawMessages) {
    emit historicalBufferReceived(rawMessages);

    // Also parse into DetectionResults for history widgets
    std::vector<DetectionResult> parsed;
    for (const auto& raw : rawMessages) {
        auto [type, payload] = Protocol::parseMessage(raw);
        if (type == Protocol::MSG_STATS && payload.contains("data")) {
            parsed.push_back(Protocol::deserializeResult(payload["data"]));
        }
    }
    if (!parsed.empty())
        emit historicalStatsReceived(parsed);
}
