/**
 * @file DatabaseManager.cpp
 * @brief Модуль взаимодействия с базой данных (реализация).
 *
 * Назначение: Реализация методов сохранения и извлечения данных из PostgreSQL.
 * Входные данные: Параметры подключения, объекты результатов классификации.
 * Результаты: Постоянное хранение данных, асинхронное логирование инцидентов.
 * Метод решения: Пакетная вставка через механизм COPY, асинхронный поток записи с таймером.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#include "common/DatabaseManager.cpp"
#include "common/DatabaseManager.hpp"
#include "common/AppLogger.hpp"
#include <QVariant>
#include <optional>
#include <sstream>
#include <iomanip>

static int s_dbConnectionId = 0;

/**
 * @brief Вспомогательная функция преобразования QDateTime в строку PostgreSQL.
 * @param dt Объект даты и времени.
 * @return Строка формата yyyy-MM-dd HH:mm:ss.zzz.
 */
static std::string toTimestampStr(const QDateTime& dt) {
  if (!dt.isValid()) return "NOW()";
  return dt.toString("yyyy-MM-dd HH:mm:ss.zzz").toStdString();
}

// Конструктор: инициализация файлового буфера для надежности
DatabaseManager::DatabaseManager(QObject* parent)
  : QObject(parent), pendingEventsBuffer_("pending_events.jsonl") {
  ++s_dbConnectionId;
}

// Деструктор: остановка потоков и закрытие соединений
DatabaseManager::~DatabaseManager() {
  stopAsyncWriter();
  disconnect();
}

// Построение строки подключения libpqxx из параметров
std::string DatabaseManager::buildConnectionString() const {
  std::ostringstream ss;
  ss << "host=" << host_.toStdString()
     << " port=" << port_
     << " dbname=" << dbName_.toStdString()
     << " user=" << user_.toStdString()
     << " password=" << password_.toStdString();
  return ss.str();
}

/**
 * @brief Инициализация подключения к базе данных.
 */
bool DatabaseManager::connectToDatabase(const QString& host, int port,
                    const QString& dbName, const QString& user,
                    const QString& password) {
  host_ = host;
  port_ = port;
  dbName_ = dbName;
  user_ = user;
  password_ = password;

  AppLogger::get()->info("DatabaseManager: подключение к БД {} под пользователем '{}'", 
    dbName_.toStdString(), user_.toStdString());

  try {
    conn_ = std::make_unique<pqxx::connection>(buildConnectionString());
    if (!conn_->is_open()) {
      AppLogger::get()->error("DatabaseManager: ошибка подключения: соединение не открыто");
      connected_ = false;
      return false;
    }

    connected_ = true;
    AppLogger::get()->info("DatabaseManager: успешно подключено к PostgreSQL: {}", dbName.toStdString());

    ensureTables();      // Проверка структуры таблиц
    startAsyncWriter();  // Запуск фонового потока записи
    return true;
  } catch (const std::exception& e) {
    AppLogger::get()->error("DatabaseManager: исключение при подключении: {}", e.what());
    connected_ = false;
    return false;
  }
}

// Разрыв соединения и очистка ресурсов
void DatabaseManager::disconnect() {
  if (connected_) {
    stopAsyncWriter();
    try {
      if (conn_) {
        conn_->close();
        conn_.reset();
      }
    } catch (...) {}
    connected_ = false;
    AppLogger::get()->info("DatabaseManager: соединение с БД разорвано.");
  }
}

bool DatabaseManager::isConnected() const { return connected_; }

/**
 * @brief Создание необходимых таблиц в БД, если они отсутствуют.
 */
void DatabaseManager::ensureTables() {
  try {
    pqxx::work tx(*conn_);
    
    // Таблица сессий мониторинга
    tx.exec(R"(
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

    // Таблица детальных событий классификации
    tx.exec(R"(
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

    // Таблица поминутных снимков трафика
    tx.exec(R"(
      CREATE TABLE IF NOT EXISTS stats_snapshots (
        id SERIAL PRIMARY KEY,
        session_id INT REFERENCES sessions(id),
        timestamp TIMESTAMP DEFAULT NOW(),
        packets_per_s REAL,
        total_packets BIGINT,
        current_label INT
      )
    )");

    // Таблица инцидентов атак
    tx.exec(R"(
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

    tx.commit();
  } catch (const std::exception& e) {
    AppLogger::get()->error("DatabaseManager: ошибка инициализации таблиц: {}", e.what());
  }
}

/**
 * @brief Регистрация новой сессии мониторинга.
 * @return ID созданной сессии или -1 при ошибке.
 */
int DatabaseManager::createSession(const QString& iface, const QString& modelName) {
  QMutexLocker lock(&dbMutex_);
  if (!conn_ || !conn_->is_open()) return -1;

  try {
    pqxx::work tx(*conn_);
    auto row = tx.exec_params1(
      "INSERT INTO sessions (interface_name, model_name, start_time) VALUES ($1, $2, NOW()) RETURNING id",
      iface.toStdString(),
      modelName.toStdString()
    );
    tx.commit();
    int id = row[0].as<int>();
    AppLogger::get()->info("DatabaseManager: начата новая сессия ID: {}", id);
    return id;
  } catch (const std::exception& e) {
    AppLogger::get()->error("DatabaseManager: ошибка создания сессии: {}", e.what());
    return -1;
  }
}

/**
 * @brief Закрытие сессии с сохранением итоговой статистики.
 */
void DatabaseManager::closeSession(int sessionId, uint64_t totalPkts, uint64_t attacks, uint64_t benign) {
  QMutexLocker lock(&dbMutex_);
  if (sessionId <= 0) return;
  try {
    pqxx::work tx(*conn_);
    tx.exec_params(
      "UPDATE sessions SET end_time=NOW(), total_packets=$1, total_attacks=$2, total_benign=$3 WHERE id=$4",
      static_cast<int64_t>(totalPkts),
      static_cast<int64_t>(attacks),
      static_cast<int64_t>(benign),
      sessionId
    );
    tx.commit();
  } catch (const std::exception& e) {
    AppLogger::get()->error("DatabaseManager: ошибка закрытия сессии {}: {}", sessionId, e.what());
  }
}

/**
 * @brief Получение списка последних 100 сессий.
 */
std::vector<SessionInfo> DatabaseManager::getSessions() {
  QMutexLocker lock(&dbMutex_);
  std::vector<SessionInfo> result;
  if (!conn_ || !conn_->is_open()) return result;

  try {
    pqxx::nontransaction tx(*conn_);
    auto rows = tx.exec(
      "SELECT id, interface_name, model_name, start_time, end_time, total_packets, total_attacks, total_benign "
      "FROM sessions ORDER BY id DESC LIMIT 100"
    );
    for (const auto& row : rows) {
      SessionInfo s;
      s.id = row[0].as<int>();
      s.interfaceName = QString::fromStdString(row[1].as<std::string>(""));
      s.modelName = QString::fromStdString(row[2].as<std::string>(""));
      s.startTime = QDateTime::fromString(
        QString::fromStdString(row[3].as<std::string>("")), "yyyy-MM-dd HH:mm:ss");
      if (!row[4].is_null())
        s.endTime = QDateTime::fromString(
          QString::fromStdString(row[4].as<std::string>("")), "yyyy-MM-dd HH:mm:ss");
      s.totalPackets = row[5].as<uint64_t>(0);
      s.totalAttacks = row[6].as<uint64_t>(0);
      s.totalBenign = row[7].as<uint64_t>(0);
      result.push_back(s);
    }
  } catch (const std::exception& e) {
    AppLogger::get()->error("DatabaseManager: ошибка извлечения сессий: {}", e.what());
  }
  return result;
}

// Методы постановки данных в асинхронную очередь
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

/**
 * @brief Извлечение всех событий классификации для конкретной сессии.
 */
std::vector<DetectionResult> DatabaseManager::getEventsForSession(int sessionId) {
  QMutexLocker lock(&dbMutex_);
  std::vector<DetectionResult> result;
  if (!conn_ || !conn_->is_open()) return result;

  try {
    pqxx::nontransaction tx(*conn_);
    auto rows = tx.exec_params(
      "SELECT timestamp, label, confidence, pps, total_packets, features, model_name "
      "FROM events WHERE session_id=$1 ORDER BY timestamp",
      sessionId
    );
    for (const auto& row : rows) {
      DetectionResult r;
      r.timestamp = QDateTime::fromString(
        QString::fromStdString(row[0].as<std::string>("")), "yyyy-MM-dd HH:mm:ss");
      r.label = row[1].as<int>(0);
      r.confidence = row[2].as<float>(0.0f);
      r.pps = row[3].as<double>(0.0);
      r.totalPackets = row[4].as<uint64_t>(0);
      r.sessionId = sessionId;
      r.modelName = row[6].as<std::string>("");
      try {
        auto j = nlohmann::json::parse(row[5].as<std::string>("[]"));
        r.features = j.get<std::vector<double>>();
      } catch(...) {}
      result.push_back(r);
    }
  } catch (const std::exception& e) {
    AppLogger::get()->error("DatabaseManager: ошибка извлечения событий для сессии {}: {}", sessionId, e.what());
  }
  return result;
}

/**
 * @brief Получение истории инцидентов безопасности.
 */
std::vector<DetectionResult> DatabaseManager::getSecurityEvents(int limit) {
  QMutexLocker lock(&dbMutex_);
  std::vector<DetectionResult> result;
  if (!conn_ || !conn_->is_open()) return result;

  try {
    pqxx::nontransaction tx(*conn_);
    auto rows = tx.exec_params(
      "SELECT start_time, type_label, confidence, pps_max, attacker_ip, session_id "
      "FROM security_events ORDER BY start_time DESC LIMIT $1",
      limit
    );
    for (const auto& row : rows) {
      DetectionResult r;
      r.timestamp = QDateTime::fromString(
        QString::fromStdString(row[0].as<std::string>("")), "yyyy-MM-dd HH:mm:ss");
      r.label = 1;
      r.confidence = row[2].as<float>(0.0f);
      r.pps = row[3].as<double>(0.0);
      r.sessionId = row[5].as<int>(0);
      r.topTalkers.push_back({row[4].as<std::string>(""), 0});
      r.modelName = row[1].as<std::string>("");
      result.push_back(r);
    }
  } catch (const std::exception& e) {
    AppLogger::get()->error("DatabaseManager: ошибка извлечения инцидентов: {}", e.what());
  }
  return result;
}

/**
 * @brief Запуск фонового потока асинхронной записи.
 */
void DatabaseManager::startAsyncWriter() {
  AppLogger::get()->info("DatabaseManager: запуск асинхронного потока записи");
  writerThread_ = std::make_unique<QThread>();
  flushTimer_ = std::make_unique<QTimer>();
  flushTimer_->setInterval(FLUSH_INTERVAL_MS);
  flushTimer_->moveToThread(writerThread_.get());

  connect(writerThread_.get(), &QThread::started, flushTimer_.get(), qOverload<>(&QTimer::start));

  connect(flushTimer_.get(), &QTimer::timeout, flushTimer_.get(), [this]() {
    // Инициализация соединения во вспомогательном потоке
    if (!writerConn_ || !writerConn_->is_open()) {
      try {
        writerConn_ = std::make_unique<pqxx::connection>(buildConnectionString());
      } catch (...) { return; }
    }

    // Сброс всех накопленных данных
    flushEvents(*writerConn_);
    flushSnapshots(*writerConn_);
    flushSecurityEvents(*writerConn_);
  });

  writerThread_->start();
}

/**
 * @brief Безопасная остановка потока записи.
 */
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
  // Финальный сброс данных перед закрытием приложения
  if (conn_ && conn_->is_open()) {
    flushEvents(*conn_);
    flushSnapshots(*conn_);
    flushSecurityEvents(*conn_);
  }

  try {
    if (writerConn_) {
      writerConn_->close();
      writerConn_.reset();
    }
  } catch (...) {}
}

/**
 * @brief Пакетная запись событий с использованием COPY.
 */
void DatabaseManager::flushEvents(pqxx::connection& conn) {
  std::optional<QMutexLocker<QMutex>> lock;
  if (&conn == conn_.get()) lock.emplace(&dbMutex_);
  if (!conn.is_open()) return;

  // 1. Проверка файлового буфера (данные, не записанные ранее из-за сбоев)
  if (pendingEventsBuffer_.size() > 0) {
    auto bufferedMessages = pendingEventsBuffer_.readAllAndClear();
    if (!bufferedMessages.empty()) {
      try {
        pqxx::work tx(conn);
        auto stream = pqxx::stream_to::table(tx, {"events"},
          {"session_id", "timestamp", "label", "confidence", "pps", "total_packets", "features", "model_name"});

        for (const auto& msg : bufferedMessages) {
          try {
            auto j = nlohmann::json::parse(msg.toStdString());
            DetectionResult r = Protocol::deserializeResult(j);
            if (r.sessionId <= 0) continue;
            stream << std::make_tuple(r.sessionId, toTimestampStr(r.timestamp), r.label, r.confidence,
                        r.pps, static_cast<int64_t>(r.totalPackets),
                        nlohmann::json(r.features).dump(), r.modelName);
          } catch (...) {}
        }
        stream.complete();
        tx.commit();
      } catch (...) {}
    }
  }

  // 2. Сброс текущей очереди
  EventEntry entry;
  std::vector<EventEntry> batch;
  while (eventQueue_.try_dequeue(entry)) {
    if (entry.result.sessionId <= 0) continue;
    batch.push_back(entry);
    if (static_cast<int>(batch.size()) >= MAX_EVENTS_PER_FLUSH) break;
  }

  if (!batch.empty()) {
    try {
      pqxx::work tx(conn);
      auto stream = pqxx::stream_to::table(tx, {"events"},
        {"session_id", "timestamp", "label", "confidence", "pps", "total_packets", "features", "model_name"});

      for (const auto& e : batch) {
        stream << std::make_tuple(
          e.result.sessionId,
          toTimestampStr(e.result.timestamp),
          e.result.label,
          e.result.confidence,
          e.result.pps,
          static_cast<int64_t>(e.result.totalPackets),
          nlohmann::json(e.result.features).dump(),
          e.result.modelName
        );
      }
      stream.complete();
      tx.commit();
    } catch (const std::exception& e) {
      AppLogger::get()->error("DatabaseManager: ошибка записи пакета событий: {}", e.what());
    }
  }
}

/**
 * @brief Пакетная запись снимков трафика.
 */
void DatabaseManager::flushSnapshots(pqxx::connection& conn) {
  std::optional<QMutexLocker<QMutex>> lock;
  if (&conn == conn_.get()) lock.emplace(&dbMutex_);
  if (!conn.is_open()) return;

  SnapshotEntry entry;
  std::vector<SnapshotEntry> batch;
  while (snapshotQueue_.try_dequeue(entry)) {
    if (entry.sessionId <= 0) continue;
    batch.push_back(entry);
    if (static_cast<int>(batch.size()) >= MAX_EVENTS_PER_FLUSH) break;
  }

  if (!batch.empty()) {
    try {
      pqxx::work tx(conn);
      auto stream = pqxx::stream_to::table(tx, {"stats_snapshots"},
        {"session_id", "timestamp", "packets_per_s", "total_packets", "current_label"});

      for (const auto& s : batch) {
        stream << std::make_tuple(s.sessionId, toTimestampStr(s.timestamp), s.pps,
                    static_cast<int64_t>(s.totalPackets), s.currentLabel);
      }
      stream.complete();
      tx.commit();
    } catch (...) {}
  }
}

/**
 * @brief Пакетная запись инцидентов безопасности.
 */
void DatabaseManager::flushSecurityEvents(pqxx::connection& conn) {
  std::optional<QMutexLocker<QMutex>> lock;
  if (&conn == conn_.get()) lock.emplace(&dbMutex_);
  if (!conn.is_open()) return;

  SecurityEventEntry entry;
  std::vector<SecurityEventEntry> batch;
  while (securityEventQueue_.try_dequeue(entry)) {
    if (entry.sessionId <= 0) continue;
    batch.push_back(entry);
    if (static_cast<int>(batch.size()) >= MAX_EVENTS_PER_FLUSH) break;
  }

  if (!batch.empty()) {
    try {
      pqxx::work tx(conn);
      auto stream = pqxx::stream_to::table(tx, {"security_events"},
        {"session_id", "start_time", "duration_sec", "attacker_ip", "pps_max", "type_label", "confidence"});

      for (const auto& se : batch) {
        stream << std::make_tuple(
          se.sessionId,
          toTimestampStr(se.startTime),
          se.duration,
          se.attackerIp.toStdString(),
          se.ppsMax,
          se.typeLabel.toStdString(),
          se.confidence
        );
      }
      stream.complete();
      tx.commit();
    } catch (...) {}
  }
}
