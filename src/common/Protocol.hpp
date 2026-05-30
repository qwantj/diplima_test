/**
 * @file Protocol.hpp
 * @brief Модуль сетевого протокола (заголовок).
 *
 * Назначение: Определение структур данных и методов сериализации для обмена между коллектором и монитором.
 * Входные данные: Объекты DetectionResult, JSON сообщения.
 * Результаты: Бинарные пакеты (QByteArray) для передачи по TCP.
 * Метод решения: Использование библиотеки nlohmann/json для сериализации и QDataStream для фрейминга.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#pragma once // Директива защиты от повторного включения

#include <nlohmann/json.hpp>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <cstdint>
#include <string>
#include <vector>
#include <map>

// ---- Основные структуры данных (в глобальном пространстве имен) ----

/**
 * @enum TrafficLabel
 * @brief Метки типа трафика.
 */
enum class TrafficLabel {
  Benign = 0, // Нормальный
  Attack = 1  // Атака
};

/**
 * @struct DetectionResult
 * @brief Результат анализа одного окна трафика.
 */
struct DetectionResult {
  QDateTime   timestamp;        // Временная метка
  int     label = 0;        // 0 - норма, 1 - атака
  float       confidence = 0.0f; // Уверенность классификатора [0..1]
  double      pps = 0.0;        // Пакетов в секунду
  uint64_t    totalPackets = 0; // Всего пакетов в окне
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

  // Вектор признаков (набор из 16 параметров)
  std::vector<double> features;

  // Дополнительная телеметрия для GUI
  std::map<std::string, double> protocolBreakdown;
  std::vector<int>        packetSizeHistogram; 
  std::vector<std::pair<std::string, uint64_t>> topTalkers;
  std::vector<std::pair<uint16_t, uint64_t>>    topPorts;
  std::vector<std::pair<std::string, uint64_t>> topTargets;
  uint32_t uniqueSourceCount = 0;
  uint32_t activeFlowsCount = 0;
  std::vector<std::string> blockedIps;

  int     sessionId = 0;    // ID сессии в БД
  std::string modelName;        // Имя используемой модели
};

/**
 * @struct SessionInfo
 * @brief Информация о сессии мониторинга.
 */
struct SessionInfo {
  int     id = 0;
  QString   interfaceName;
  QString   modelName;
  QDateTime startTime;
  QDateTime endTime;
  uint64_t  totalPackets = 0;
  uint64_t  totalAttacks = 0;
  uint64_t  totalBenign  = 0;
};

/**
 * @namespace Protocol
 * @brief Служебные функции для работы с сетевым протоколом.
 */
namespace Protocol {

const std::string PROTOCOL_VERSION = "2.2";

// Типы сообщений
constexpr const char* MSG_STATS    = "stats";
constexpr const char* MSG_SNAPSHOT = "snapshot";
constexpr const char* MSG_CMD      = "cmd";
constexpr const char* MSG_NOTIFY   = "notify";
constexpr const char* MSG_BUFFER   = "buffer";

// Команды управления
constexpr const char* CMD_LOAD_PCAP   = "load_pcap";
constexpr const char* CMD_STOP_REPLAY = "stop_replay";
constexpr const char* CMD_LOAD_MODEL  = "load_model";
constexpr const char* CMD_CONFIG_BPF  = "config_bpf";
constexpr const char* CMD_CONFIG_DUMP = "config_dump";
constexpr const char* CMD_STOP        = "stop";

// Сериализация/десериализация в JSON
nlohmann::json serializeResult(const DetectionResult& r);
DetectionResult deserializeResult(const nlohmann::json& j);

// Генерация сообщений для передачи по сети
QByteArray serializeSnapshot(float pps, uint64_t totalPackets, int currentLabel);
QByteArray serializeStats(const DetectionResult& r, uint64_t totalPackets);
QByteArray serializeCommand(const std::string& cmd, const nlohmann::json& data = {});
QByteArray serializeNotify(const std::string& event, const nlohmann::json& data = {});

// Парсинг входящего пакета
std::pair<std::string, nlohmann::json> parseMessage(const QByteArray& raw);

// Оборачивание данных в длину (length-prefixing)
QByteArray frame(const QByteArray& payload);

} // namespace Protocol

// Регистрация типов в метасистеме Qt
Q_DECLARE_METATYPE(DetectionResult)
Q_DECLARE_METATYPE(SessionInfo)
Q_DECLARE_METATYPE(std::vector<DetectionResult>)
Q_DECLARE_METATYPE(std::vector<SessionInfo>)
Q_DECLARE_METATYPE(std::vector<QByteArray>)
