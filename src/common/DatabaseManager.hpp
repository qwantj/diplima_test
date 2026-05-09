#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QDateTime>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <vector>
#include <memory>

#include "common/Protocol.hpp"
#include "concurrentqueue.h"

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager() override;

    bool connectToDatabase(const QString& host = "localhost",
                           int port = 5432,
                           const QString& dbName = "ddos_detection_db",
                           const QString& user = "postgres",
                           const QString& password = "qwerty");
    void disconnect();
    bool isConnected() const;

    // Session management
    int  createSession(const QString& iface, const QString& modelName);
    void closeSession(int sessionId, uint64_t totalPkts, uint64_t attacks, uint64_t benign);
    std::vector<SessionInfo> getSessions();

    // Event insertion (queued, async)
    void enqueueEvent(const DetectionResult& result);
    void enqueueSnapshot(float pps, uint64_t totalPackets, int currentLabel, int sessionId);

    // History retrieval
    std::vector<DetectionResult> getEventsForSession(int sessionId);
    std::vector<DetectionResult> getSecurityEvents(int limit = 100);

private:
    void ensureTables();
    void startAsyncWriter();
    void stopAsyncWriter();
    void flushEvents();
    void flushSnapshots();

    QSqlDatabase db_;
    QString connectionName_;
    bool connected_ = false;

    // Async writer
    struct EventEntry {
        DetectionResult result;
    };
    struct SnapshotEntry {
        float pps;
        uint64_t totalPackets;
        int currentLabel;
        int sessionId;
        QDateTime timestamp;
    };

    moodycamel::ConcurrentQueue<EventEntry> eventQueue_;
    moodycamel::ConcurrentQueue<SnapshotEntry> snapshotQueue_;
    std::unique_ptr<QThread> writerThread_;
    std::unique_ptr<QTimer>  flushTimer_;
    QMutex dbMutex_;
    static constexpr int FLUSH_INTERVAL_MS = 5000;
};
