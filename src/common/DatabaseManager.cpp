#include "common/DatabaseManager.hpp"
#include "common/AppLogger.hpp"
#include <QSqlRecord>
#include <QVariant>
#include <optional>

static int s_dbConnectionId = 0;

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent), pendingEventsBuffer_("pending_events.jsonl")
{
    connectionName_ = QString("ddos_db_%1").arg(++s_dbConnectionId);
}

DatabaseManager::~DatabaseManager() {
    stopAsyncWriter();
    disconnect();
}

bool DatabaseManager::connectToDatabase(const QString& host, int port,
                                         const QString& dbName, const QString& user,
                                         const QString& password) {
    host_ = host;
    port_ = port;
    dbName_ = dbName;
    user_ = user;
    password_ = password;

    db_ = QSqlDatabase::addDatabase("QPSQL", connectionName_);
    db_.setHostName(host_);
    db_.setPort(port_);
    db_.setDatabaseName(dbName_);
    db_.setUserName(user_);
    db_.setPassword(password_);

    AppLogger::get()->info("Connecting to DB with user: '{}', password length: {}", user_.toStdString(), password_.length());

    if (!db_.open()) {
        AppLogger::get()->error("Database connection error: {} {}",
            db_.lastError().text().toStdString(),
            db_.lastError().driverText().toStdString());
        connected_ = false;
        return false;
    }

    connected_ = true;
    AppLogger::get()->info("Successfully connected to PostgreSQL database: {}",
        dbName.toStdString());

    ensureTables();
    startAsyncWriter();
    return true;
}

void DatabaseManager::disconnect() {
    if (connected_) {
        stopAsyncWriter();
        db_.close();
        connected_ = false;
        db_ = QSqlDatabase(); 
        QSqlDatabase::removeDatabase(connectionName_);
        AppLogger::get()->info("Disconnected from database.");
    }
}

bool DatabaseManager::isConnected() const { return connected_; }

void DatabaseManager::ensureTables() {
    QSqlQuery q(db_);
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS sessions (
            id SERIAL PRIMARY KEY,
            interface_name TEXT,
            model_name TEXT,
            start_time TIMESTAMP DEFAULT NOW(),
            end_time TIMESTAMP,
            total_packets BIGINT DEFAULT 0,
            total_attacks BIGINT DEFAULT 0,
            total_benign BIGINT DEFAULT 0
        )
    )");
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS events (
            id SERIAL PRIMARY KEY,
            session_id INT REFERENCES sessions(id),
            timestamp TIMESTAMP DEFAULT NOW(),
            label INT,
            confidence REAL,
            pps REAL,
            total_packets BIGINT,
            features JSONB,
            model_name TEXT
        )
    )");
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS stats_snapshots (
            id SERIAL PRIMARY KEY,
            session_id INT REFERENCES sessions(id),
            timestamp TIMESTAMP DEFAULT NOW(),
            packets_per_s REAL,
            total_packets BIGINT,
            current_label INT
        )
    )");
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS security_events (
            id SERIAL PRIMARY KEY,
            session_id INT REFERENCES sessions(id),
            start_time TIMESTAMP,
            duration_sec REAL,
            attacker_ip TEXT,
            pps_max REAL,
            type_label TEXT,
            confidence REAL
        )
    )");
}

int DatabaseManager::createSession(const QString& iface, const QString& modelName) {
    QMutexLocker lock(&dbMutex_);
    if (!db_.isOpen()) {
        AppLogger::get()->error("DatabaseManager: cannot create session, DB not open");
        return -1;
    }

    QSqlQuery q(db_);
    q.prepare("INSERT INTO sessions (interface_name, model_name, start_time) VALUES (?, ?, NOW()) RETURNING id");
    q.addBindValue(iface);
    q.addBindValue(modelName);
    
    if (q.exec()) {
        if (q.next()) {
            int id = q.value(0).toInt();
            AppLogger::get()->info("DatabaseManager: created new session ID: {}", id);
            return id;
        }
    }
    
    AppLogger::get()->error("DatabaseManager: failed to create session: {}", q.lastError().text().toStdString());
    return -1;
}

void DatabaseManager::closeSession(int sessionId, uint64_t totalPkts, uint64_t attacks, uint64_t benign) {
    QMutexLocker lock(&dbMutex_);
    if (sessionId <= 0) return;
    QSqlQuery q(db_);
    q.prepare("UPDATE sessions SET end_time=NOW(), total_packets=?, total_attacks=?, total_benign=? WHERE id=?");
    q.addBindValue((qint64)totalPkts);
    q.addBindValue((qint64)attacks);
    q.addBindValue((qint64)benign);
    q.addBindValue(sessionId);
    q.exec();
}

std::vector<SessionInfo> DatabaseManager::getSessions() {
    QMutexLocker lock(&dbMutex_);
    std::vector<SessionInfo> result;
    if (!db_.isOpen()) return result;

    QSqlQuery q(db_);
    q.exec("SELECT id, interface_name, model_name, start_time, end_time, total_packets, total_attacks, total_benign FROM sessions ORDER BY id DESC LIMIT 100");
    while (q.next()) {
        SessionInfo s;
        s.id = q.value(0).toInt();
        s.interfaceName = q.value(1).toString();
        s.modelName = q.value(2).toString();
        s.startTime = q.value(3).toDateTime();
        s.endTime = q.value(4).toDateTime();
        s.totalPackets = q.value(5).toULongLong();
        s.totalAttacks = q.value(6).toULongLong();
        s.totalBenign = q.value(7).toULongLong();
        result.push_back(s);
    }
    return result;
}

void DatabaseManager::enqueueEvent(const DetectionResult& result) {
    eventQueue_.enqueue({result});
}

void DatabaseManager::enqueueSnapshot(float pps, uint64_t totalPackets, int currentLabel, int sessionId) {
    snapshotQueue_.enqueue({pps, totalPackets, currentLabel, sessionId, QDateTime::currentDateTime()});
}

void DatabaseManager::enqueueSecurityEvent(int sessionId, const QDateTime& startTime, float duration, 
                                          const QString& attackerIp, float ppsMax, 
                                          const QString& typeLabel, float confidence) {
    securityEventQueue_.enqueue({sessionId, startTime, duration, attackerIp, ppsMax, typeLabel, confidence});
}

std::vector<DetectionResult> DatabaseManager::getEventsForSession(int sessionId) {
    QMutexLocker lock(&dbMutex_);
    std::vector<DetectionResult> result;
    if (!db_.isOpen()) return result;

    QSqlQuery q(db_);
    q.prepare("SELECT timestamp, label, confidence, pps, total_packets, features, model_name FROM events WHERE session_id=? ORDER BY timestamp");
    q.addBindValue(sessionId);
    if (!q.exec()) return result;
    while (q.next()) {
        DetectionResult r;
        r.timestamp = q.value(0).toDateTime();
        r.label = q.value(1).toInt();
        r.confidence = q.value(2).toFloat();
        r.pps = q.value(3).toDouble();
        r.totalPackets = q.value(4).toULongLong();
        r.sessionId = sessionId;
        r.modelName = q.value(6).toString().toStdString();
        try {
            auto j = nlohmann::json::parse(q.value(5).toString().toStdString());
            r.features = j.get<std::vector<double>>();
        } catch(const std::exception& e) {
            AppLogger::get()->error("Failed to parse features JSON for session {}: {}", sessionId, e.what());
        }
        result.push_back(r);
    }
    return result;
}

std::vector<DetectionResult> DatabaseManager::getSecurityEvents(int limit) {
    QMutexLocker lock(&dbMutex_);
    std::vector<DetectionResult> result;
    if (!db_.isOpen()) return result;

    QSqlQuery q(db_);
    q.prepare("SELECT start_time, type_label, confidence, pps_max, attacker_ip, session_id FROM security_events ORDER BY start_time DESC LIMIT ?");
    q.addBindValue(limit);
    if (!q.exec()) return result;
    while (q.next()) {
        DetectionResult r;
        r.timestamp = q.value(0).toDateTime();
        r.label = 1;
        r.confidence = q.value(2).toFloat();
        r.pps = q.value(3).toDouble();
        r.sessionId = q.value(5).toInt();
        
        // Use topTalkers[0].first for attacker_ip and modelName for type_label
        r.topTalkers.push_back({q.value(4).toString().toStdString(), 0});
        r.modelName = q.value(1).toString().toStdString();
        
        result.push_back(r);
    }
    return result;
}

void DatabaseManager::startAsyncWriter() {
    AppLogger::get()->info("DatabaseManager: async writer started");
    writerThread_ = std::make_unique<QThread>();
    flushTimer_ = std::make_unique<QTimer>();
    flushTimer_->setInterval(FLUSH_INTERVAL_MS);
    flushTimer_->moveToThread(writerThread_.get());

    QString threadConnectionName = connectionName_ + "_writer";
    connect(writerThread_.get(), &QThread::started, flushTimer_.get(), qOverload<>(&QTimer::start));

    connect(flushTimer_.get(), &QTimer::timeout, flushTimer_.get(), [this, threadConnectionName]() {
        if (!QSqlDatabase::contains(threadConnectionName)) {
            QSqlDatabase threadDb = QSqlDatabase::addDatabase("QPSQL", threadConnectionName);
            threadDb.setHostName(host_);
            threadDb.setPort(port_);
            threadDb.setDatabaseName(dbName_);
            threadDb.setUserName(user_);
            threadDb.setPassword(password_);
            if (!threadDb.open()) {
                AppLogger::get()->error("DatabaseManager writer thread connection error: {}", threadDb.lastError().text().toStdString());
                return;
            }
        }

        QSqlDatabase threadDb = QSqlDatabase::database(threadConnectionName);
        flushEvents(threadDb);
        flushSnapshots(threadDb);
        flushSecurityEvents(threadDb);
    });

    writerThread_->start();
}

void DatabaseManager::stopAsyncWriter() {
    if (writerThread_ && writerThread_->isRunning()) {
        QThread* current = QThread::currentThread();
        QMetaObject::invokeMethod(flushTimer_.get(), [this, current]() {
            flushTimer_->stop();
            flushTimer_->moveToThread(current);
        }, Qt::BlockingQueuedConnection);
        writerThread_->quit();
        writerThread_->wait();
    }
    // Final flush
    if (db_.isOpen()) {
        flushEvents(db_);
        flushSnapshots(db_);
        flushSecurityEvents(db_);
    }

    QString threadConnectionName = connectionName_ + "_writer";
    if (QSqlDatabase::contains(threadConnectionName)) {
        QSqlDatabase::removeDatabase(threadConnectionName);
    }
}

void DatabaseManager::flushEvents(QSqlDatabase& db) {
    std::optional<QMutexLocker<QMutex>> lock;
    if (db.connectionName() == connectionName_) {
        lock.emplace(&dbMutex_);
    }

    if (!db.isOpen()) return;

    // 1. Process pending events from disk buffer
    if (pendingEventsBuffer_.size() > 0) {
        auto bufferedMessages = pendingEventsBuffer_.readAllAndClear();
        if (!bufferedMessages.empty()) {
            QVariantList sessionIds, timestamps, labels, confidences, ppsValues, totalPackets, features, modelNames;
            int count = 0;
            for (const auto& msg : bufferedMessages) {
                try {
                    auto j = nlohmann::json::parse(msg.toStdString());
                    DetectionResult r = Protocol::deserializeResult(j);
                    if (r.sessionId <= 0) continue;
                    sessionIds << r.sessionId;
                    timestamps << r.timestamp;
                    labels << r.label;
                    confidences << r.confidence;
                    ppsValues << r.pps;
                    totalPackets << static_cast<qint64>(r.totalPackets);
                    features << QString::fromStdString(nlohmann::json(r.features).dump());
                    modelNames << QString::fromStdString(r.modelName);
                    count++;
                } catch (...) {}
            }
            if (count > 0 && db.transaction()) {
                QSqlQuery q(db);
                q.prepare("INSERT INTO events (session_id, timestamp, label, confidence, pps, total_packets, features, model_name) VALUES (?,?,?,?,?,?,?,?)");
                q.bindValue(0, sessionIds); q.bindValue(1, timestamps); q.bindValue(2, labels); q.bindValue(3, confidences);
                q.bindValue(4, ppsValues); q.bindValue(5, totalPackets); q.bindValue(6, features); q.bindValue(7, modelNames);
                if (q.execBatch()) db.commit(); else { db.rollback(); }
            }
        }
    }

    // 2. Process live queue
    EventEntry entry;
    QVariantList sessionIds, timestamps, labels, confidences, ppsValues, totalPackets, features, modelNames;
    int liveCount = 0;
    while (eventQueue_.try_dequeue(entry)) {
        if (entry.result.sessionId <= 0) continue;
        sessionIds << entry.result.sessionId;
        timestamps << entry.result.timestamp;
        labels << entry.result.label;
        confidences << entry.result.confidence;
        ppsValues << entry.result.pps;
        totalPackets << static_cast<qint64>(entry.result.totalPackets);
        features << QString::fromStdString(nlohmann::json(entry.result.features).dump());
        modelNames << QString::fromStdString(entry.result.modelName);
        liveCount++;
        if (liveCount >= MAX_EVENTS_PER_FLUSH) break;
    }

    if (liveCount > 0) {
        if (db.transaction()) {
            QSqlQuery q(db);
            if (q.prepare("INSERT INTO events (session_id, timestamp, label, confidence, pps, total_packets, features, model_name) VALUES (?,?,?,?,?,?,?,?)")) {
                q.bindValue(0, sessionIds); q.bindValue(1, timestamps); q.bindValue(2, labels); q.bindValue(3, confidences);
                q.bindValue(4, ppsValues); q.bindValue(5, totalPackets); q.bindValue(6, features); q.bindValue(7, modelNames);
                if (q.execBatch()) {
                    db.commit();
                } else {
                    AppLogger::get()->error("DatabaseManager: events batch failed: {}", q.lastError().text().toStdString());
                    db.rollback();
                }
            } else {
                db.rollback();
            }
        }
    }
}

void DatabaseManager::flushSnapshots(QSqlDatabase& db) {
    std::optional<QMutexLocker<QMutex>> lock;
    if (db.connectionName() == connectionName_) lock.emplace(&dbMutex_);
    if (!db.isOpen()) return;

    SnapshotEntry entry;
    QVariantList sessionIds, timestamps, ppsValues, totalPackets, labels;
    int count = 0;
    while (snapshotQueue_.try_dequeue(entry)) {
        if (entry.sessionId <= 0) continue;
        sessionIds << entry.sessionId;
        timestamps << entry.timestamp;
        ppsValues << entry.pps;
        totalPackets << static_cast<qint64>(entry.totalPackets);
        labels << entry.currentLabel;
        count++;
        if (count >= MAX_EVENTS_PER_FLUSH) break;
    }

    if (count > 0 && db.transaction()) {
        QSqlQuery q(db);
        if (q.prepare("INSERT INTO stats_snapshots (session_id, timestamp, packets_per_s, total_packets, current_label) VALUES (?,?,?,?,?)")) {
            q.bindValue(0, sessionIds); q.bindValue(1, timestamps); q.bindValue(2, ppsValues); q.bindValue(3, totalPackets); q.bindValue(4, labels);
            if (q.execBatch()) db.commit(); else { 
                AppLogger::get()->error("DatabaseManager: snapshots batch failed: {}", q.lastError().text().toStdString());
                db.rollback(); 
            }
        } else { db.rollback(); }
    }
}

void DatabaseManager::flushSecurityEvents(QSqlDatabase& db) {
    std::optional<QMutexLocker<QMutex>> lock;
    if (db.connectionName() == connectionName_) lock.emplace(&dbMutex_);
    if (!db.isOpen()) return;

    SecurityEventEntry entry;
    QVariantList sessionIds, startTimes, durations, attackerIps, ppsMaxs, typeLabels, confidences;
    int count = 0;
    while (securityEventQueue_.try_dequeue(entry)) {
        if (entry.sessionId <= 0) continue;
        sessionIds << entry.sessionId;
        startTimes << entry.startTime;
        durations << entry.duration;
        attackerIps << entry.attackerIp;
        ppsMaxs << entry.ppsMax;
        typeLabels << entry.typeLabel;
        confidences << entry.confidence;
        count++;
        if (count >= MAX_EVENTS_PER_FLUSH) break;
    }

    if (count > 0 && db.transaction()) {
        QSqlQuery q(db);
        if (q.prepare("INSERT INTO security_events (session_id, start_time, duration_sec, attacker_ip, pps_max, type_label, confidence) VALUES (?,?,?,?,?,?,?)")) {
            q.bindValue(0, sessionIds); q.bindValue(1, startTimes); q.bindValue(2, durations); q.bindValue(3, attackerIps);
            q.bindValue(4, ppsMaxs); q.bindValue(5, typeLabels); q.bindValue(6, confidences);
            if (q.execBatch()) db.commit(); else { 
                AppLogger::get()->error("DatabaseManager: security events batch failed: {}", q.lastError().text().toStdString());
                db.rollback(); 
            }
        } else { db.rollback(); }
    }
}
