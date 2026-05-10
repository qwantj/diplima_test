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
#include "common/FileBuffer.hpp"
#include "concurrentqueue.h"

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager() override;

    bool connectToDatabase(const QString& host,
                           int port,
                           const QString& dbName,
                           const QString& user,
                           const QString& password);
    void disconnect();
    bool isConnected() const;

    // Session management
    int  createSession(const QString& iface, const QString& modelName);
    void closeSession(int sessionId, uint64_t totalPkts, uint64_t attacks, uint64_t benign);
    std::vector<SessionInfo> getSessions();

    // Event insertion (queued, async)
    void enqueueEvent(const DetectionResult& result);
    void enqueueSnapshot(float pps, uint64_t totalPackets, int currentLabel, int sessionId);
    void enqueueSecurityEvent(int sessionId, const QDateTime& startTime, float duration, 
                              const QString& attackerIp, float ppsMax, 
                              const QString& typeLabel, float confidence);

    // History retrieval
    std::vector<DetectionResult> getEventsForSession(int sessionId);
    std::vector<DetectionResult> getSecurityEvents(int limit = 100);

private:
    void ensureTables();
    void startAsyncWriter();
    void stopAsyncWriter();
    void flushEvents(QSqlDatabase& db);
    void flushSnapshots(QSqlDatabase& db);
    void flushSecurityEvents(QSqlDatabase& db);

    QSqlDatabase db_;
    QString connectionName_;
    QString host_, dbName_, user_, password_;
    int port_ = 5432;
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
    struct SecurityEventEntry {
        int sessionId;
        QDateTime startTime;
        float duration;
        QString attackerIp;
        float ppsMax;
        QString typeLabel;
        float confidence;
    };

    moodycamel::ConcurrentQueue<EventEntry> eventQueue_;
    moodycamel::ConcurrentQueue<SnapshotEntry> snapshotQueue_;
    moodycamel::ConcurrentQueue<SecurityEventEntry> securityEventQueue_;
    std::unique_ptr<QThread> writerThread_;
    std::unique_ptr<QTimer>  flushTimer_;
    QMutex dbMutex_;
    static constexpr int FLUSH_INTERVAL_MS = 5000;

    // Database Throttling State
    QDateTime lastEventFlushTime_;
    int eventBufferCount_ = 0;
    static constexpr int MAX_EVENTS_PER_FLUSH = 1000;

    FileBuffer pendingEventsBuffer_;
};
