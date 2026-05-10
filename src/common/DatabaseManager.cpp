#include "common/DatabaseManager.hpp"
#include "common/AppLogger.hpp"
#include <QSqlRecord>
#include <QVariant>

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
    QSqlQuery q(db_);
    q.prepare("INSERT INTO sessions (interface_name, model_name) VALUES (?, ?) RETURNING id");
    q.addBindValue(iface);
    q.addBindValue(modelName);
    if (q.exec() && q.next())
        return q.value(0).toInt();
    return -1;
}

void DatabaseManager::closeSession(int sessionId, uint64_t totalPkts, uint64_t attacks, uint64_t benign) {
    QMutexLocker lock(&dbMutex_);
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
        } catch(...) {}
        result.push_back(r);
    }
    return result;
}

std::vector<DetectionResult> DatabaseManager::getSecurityEvents(int limit) {
    QMutexLocker lock(&dbMutex_);
    std::vector<DetectionResult> result;
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
        result.push_back(r);
    }
    return result;
}

void DatabaseManager::startAsyncWriter() {
    AppLogger::get()->info("DatabaseManager: async writer started (flush every {} ms)", FLUSH_INTERVAL_MS);
    writerThread_ = std::make_unique<QThread>();
    flushTimer_ = std::make_unique<QTimer>();
    flushTimer_->setInterval(FLUSH_INTERVAL_MS);
    flushTimer_->moveToThread(writerThread_.get());

    QString threadConnectionName = connectionName_ + "_writer";
    connect(writerThread_.get(), &QThread::started, flushTimer_.get(),
        qOverload<>(&QTimer::start));

    connect(flushTimer_.get(), &QTimer::timeout, flushTimer_.get(), [this, threadConnectionName]() {
        if (!QSqlDatabase::contains(threadConnectionName)) {
            QSqlDatabase threadDb = QSqlDatabase::addDatabase("QPSQL", threadConnectionName);
            threadDb.setHostName(host_);
            threadDb.setPort(port_);
            threadDb.setDatabaseName(dbName_);
            threadDb.setUserName(user_);
            threadDb.setPassword(password_);
            if (!threadDb.open()) {
                AppLogger::get()->error("DatabaseManager writer thread connection error");
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
    flushEvents(db_);
    flushSnapshots(db_);
    flushSecurityEvents(db_);

    QString threadConnectionName = connectionName_ + "_writer";
    if (QSqlDatabase::contains(threadConnectionName)) {
        QSqlDatabase::removeDatabase(threadConnectionName);
    }
}

void DatabaseManager::flushEvents(QSqlDatabase& db) {
    std::unique_ptr<QMutexLocker<QMutex>> lock;
    if (db.connectionName() == connectionName_) {
        lock = std::make_unique<QMutexLocker<QMutex>>(&dbMutex_);
    }

    int count = 0;
    
    // 1. Process pending events from disk buffer first (if any)
    if (pendingEventsBuffer_.size() > 0 && db.isOpen()) {
        auto bufferedMessages = pendingEventsBuffer_.readAllAndClear();
        if (!bufferedMessages.empty()) {
            db.transaction();
            for (const auto& msg : bufferedMessages) {
                try {
                    auto j = nlohmann::json::parse(msg.toStdString());
                    DetectionResult r = Protocol::deserializeResult(j);
                    
                    QSqlQuery q(db);
                    q.prepare("INSERT INTO events (session_id, timestamp, label, confidence, pps, total_packets, features, model_name) VALUES (?,?,?,?,?,?,?,?)");
                    q.addBindValue(r.sessionId);
                    q.addBindValue(r.timestamp);
                    q.addBindValue(r.label);
                    q.addBindValue(r.confidence);
                    q.addBindValue(r.pps);
                    q.addBindValue((qint64)r.totalPackets);
                    nlohmann::json featJson = r.features;
                    q.addBindValue(QString::fromStdString(featJson.dump()));
                    q.addBindValue(QString::fromStdString(r.modelName));
                    q.exec();
                    ++count;
                } catch (...) {}
            }
            db.commit();
            AppLogger::get()->info("DatabaseManager: restored and flushed {} buffered offline events.", count);
            count = 0; // reset counter for the live queue
        }
    }

    // 2. Process live queue
    EventEntry entry;
    int throttled = 0;
    bool transactionActive = false;
    
    while (eventQueue_.try_dequeue(entry)) {
        if (!db.isOpen()) {
            // DB disconnected: save to disk buffer
            nlohmann::json j = Protocol::serializeResult(entry.result);
            pendingEventsBuffer_.push(QByteArray::fromStdString(j.dump()));
            continue;
        }

        if (count >= MAX_EVENTS_PER_FLUSH) {
            // Throttling: save excess to disk buffer
            nlohmann::json j = Protocol::serializeResult(entry.result);
            pendingEventsBuffer_.push(QByteArray::fromStdString(j.dump()));
            throttled++;
            continue;
        }
        
        if (!transactionActive) {
            db.transaction();
            transactionActive = true;
        }

        QSqlQuery q(db);
        q.prepare("INSERT INTO events (session_id, timestamp, label, confidence, pps, total_packets, features, model_name) VALUES (?,?,?,?,?,?,?,?)");
        q.addBindValue(entry.result.sessionId);
        q.addBindValue(entry.result.timestamp);
        q.addBindValue(entry.result.label);
        q.addBindValue(entry.result.confidence);
        q.addBindValue(entry.result.pps);
        q.addBindValue((qint64)entry.result.totalPackets);
        nlohmann::json featJson = entry.result.features;
        q.addBindValue(QString::fromStdString(featJson.dump()));
        q.addBindValue(QString::fromStdString(entry.result.modelName));
        q.exec();
        ++count;
    }
    
    if (transactionActive) {
        db.commit();
    }

    if (throttled > 0) {
        AppLogger::get()->warn("DatabaseManager: throttled {} events to disk buffer to protect DB.", throttled);
    }
}

void DatabaseManager::flushSnapshots(QSqlDatabase& db) {
    std::unique_ptr<QMutexLocker<QMutex>> lock;
    if (db.connectionName() == connectionName_) {
        lock = std::make_unique<QMutexLocker<QMutex>>(&dbMutex_);
    }

    SnapshotEntry entry;
    int count = 0;
    db.transaction();
    while (snapshotQueue_.try_dequeue(entry)) {
        QSqlQuery q(db);
        q.prepare("INSERT INTO stats_snapshots (session_id, timestamp, packets_per_s, total_packets, current_label) VALUES (?,?,?,?,?)");
        q.addBindValue(entry.sessionId);
        q.addBindValue(entry.timestamp);
        q.addBindValue(entry.pps);
        q.addBindValue((qint64)entry.totalPackets);
        q.addBindValue(entry.currentLabel);
        q.exec();
        ++count;
    }
    db.commit();
    if (count > 0)
        AppLogger::get()->info("DatabaseManager: flushed {} stats snapshots.", count);
}

void DatabaseManager::flushSecurityEvents(QSqlDatabase& db) {
    std::unique_ptr<QMutexLocker<QMutex>> lock;
    if (db.connectionName() == connectionName_) {
        lock = std::make_unique<QMutexLocker<QMutex>>(&dbMutex_);
    }

    SecurityEventEntry entry;
    int count = 0;
    db.transaction();
    while (securityEventQueue_.try_dequeue(entry)) {
        QSqlQuery q(db);
        q.prepare("INSERT INTO security_events (session_id, start_time, duration_sec, attacker_ip, pps_max, type_label, confidence) VALUES (?,?,?,?,?,?,?)");
        q.addBindValue(entry.sessionId);
        q.addBindValue(entry.startTime);
        q.addBindValue(entry.duration);
        q.addBindValue(entry.attackerIp);
        q.addBindValue(entry.ppsMax);
        q.addBindValue(entry.typeLabel);
        q.addBindValue(entry.confidence);
        q.exec();
        ++count;
    }
    db.commit();
    if (count > 0)
        AppLogger::get()->info("DatabaseManager: flushed {} security events.", count);
}
