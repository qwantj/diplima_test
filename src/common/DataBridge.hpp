#pragma once

#include <QObject>
#include <vector>
#include "common/TcpClient.hpp"
#include "common/DatabaseManager.hpp"
#include "common/Protocol.hpp"

class DataBridge : public QObject {
    Q_OBJECT
public:
    explicit DataBridge(QObject* parent = nullptr);
    ~DataBridge() override;

    void connectToCollector(const QString& host = "localhost", quint16 port = 50050);
    void connectToDatabase(const QString& host = "localhost", int port = 5432,
                           const QString& dbName = "ddos_detection_db",
                           const QString& user = "postgres",
                           const QString& password = "qwerty");

    TcpClient* tcpClient() const { return tcpClient_; }
    DatabaseManager* databaseManager() const { return dbManager_; }

signals:
    void collectorConnectionChanged(bool connected);
    void databaseConnectionChanged(bool connected);
    void realtimeStatsReceived(const DetectionResult& result, uint64_t totalPackets);
    void realtimeSnapshotReceived(float pps, uint64_t totalPackets, int currentLabel);
    void historicalBufferReceived(const std::vector<QByteArray>& rawMessages);
    void historicalStatsReceived(const std::vector<DetectionResult>& events);
    void notificationReceived(const QString& event, const QJsonObject& data);

public slots:
    void sendBpfConfig(bool enable);

private slots:
    void onCollectorConnected();
    void onCollectorDisconnected();
    void onHistoricalBufferReceived(const std::vector<QByteArray>& rawMessages);

private:
    TcpClient*       tcpClient_ = nullptr;
    DatabaseManager*  dbManager_ = nullptr;
};
