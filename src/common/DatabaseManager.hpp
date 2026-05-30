/**
 * @file DatabaseManager.hpp
 * @brief Модуль взаимодействия с базой данных (заголовок).
 *
 * Назначение: Класс для управления хранением данных в СУБД PostgreSQL.
 * Входные данные: Результаты обнаружения, сессионная информация.
 * Результаты: Сохранение данных в таблицах sessions, events, security_events.
 * Метод решения: Асинхронная запись через очереди и механизм COPY для повышения производительности.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#pragma once // Директива защиты от повторного включения

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <vector>
#include <memory>
#include <string>

#include <pqxx/pqxx>

#include "common/Protocol.hpp"
#include "common/FileBuffer.hpp"
#include "concurrentqueue.h"

/**
 * @class DatabaseManager
 * @brief Класс для работы с PostgreSQL и обеспечения отказоустойчивости хранения.
 */
class DatabaseManager : public QObject {
  Q_OBJECT
public:
  explicit DatabaseManager(QObject* parent = nullptr);
  ~DatabaseManager() override;

  // Установка соединения с БД
  bool connectToDatabase(const QString& host,
               int port,
               const QString& dbName,
               const QString& user,
               const QString& password);
  
  // Разрыв соединения
  void disconnect();
  
  // Проверка статуса подключения
  bool isConnected() const;

  // Управление сессиями мониторинга
  int  createSession(const QString& iface, const QString& modelName);
  void closeSession(int sessionId, uint64_t totalPkts, uint64_t attacks, uint64_t benign);
  std::vector<SessionInfo> getSessions();

  // Добавление событий в очереди на запись (асинхронно)
  void enqueueEvent(const DetectionResult& result);
  void enqueueSnapshot(float pps, uint64_t totalPackets, int currentLabel, int sessionId);
  void enqueueSecurityEvent(int sessionId, const QDateTime& startTime, float duration, 
                const QString& attackerIp, float ppsMax, 
                const QString& typeLabel, float confidence);

  // Методы извлечения истории для визуализации
  std::vector<DetectionResult> getEventsForSession(int sessionId);
  std::vector<DetectionResult> getSecurityEvents(int limit = 100);

private:
  // Инициализация структуры таблиц БД
  void ensureTables();
  
  // Управление вспомогательным потоком записи
  void startAsyncWriter();
  void stopAsyncWriter();
  
  // Методы пакетного сброса данных в БД
  void flushEvents(pqxx::connection& conn);
  void flushSnapshots(pqxx::connection& conn);
  void flushSecurityEvents(pqxx::connection& conn);

  // Формирование строки подключения libpqxx
  std::string buildConnectionString() const;

  std::unique_ptr<pqxx::connection> conn_;         // Соединение основного потока (чтение)
  std::unique_ptr<pqxx::connection> writerConn_;   // Соединение потока записи

  QString host_, dbName_, user_, password_;
  int port_ = 5432;
  bool connected_ = false;

  // Структуры для внутреннего хранения в очередях
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

  // Потокобезопасные очереди для асинхронного сброса данных
  moodycamel::ConcurrentQueue<EventEntry> eventQueue_;
  moodycamel::ConcurrentQueue<SnapshotEntry> snapshotQueue_;
  moodycamel::ConcurrentQueue<SecurityEventEntry> securityEventQueue_;
  
  std::unique_ptr<QThread> writerThread_; // Поток записи
  std::unique_ptr<QTimer>  flushTimer_;  // Таймер периодического сброса
  QMutex dbMutex_;                        // Защита общих данных
  static constexpr int FLUSH_INTERVAL_MS = 5000; // Интервал сброса в БД

  // Параметры пакетной обработки
  QDateTime lastEventFlushTime_;
  int eventBufferCount_ = 0;
  static constexpr int MAX_EVENTS_PER_FLUSH = 1000;

  FileBuffer pendingEventsBuffer_; // Буфер на случай потери связи с БД
};
