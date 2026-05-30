/**
 * @file FeatureExtractor.hpp
 * @brief Модуль извлечения признаков сетевого трафика (заголовок).
 *
 * Назначение: Агрегация сетевых пакетов в потоки и вычисление статистических признаков для ML-модели.
 * Входные данные: Сырые сетевые пакеты.
 * Результаты: Вектор нормализованных признаков (z-score).
 * Метод решения: Использование скользящего окна 1с, 5-tuple идентификация потоков, расчет энтропии и флагов TCP.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#pragma once // Директива защиты от повторного включения

#include <vector>
#include <map>
#include <set>
#include <string>
#include <cstdint>
#include <chrono>
#include <tuple>

#include <RawPacket.h>
#include <nlohmann/json.hpp>

#include "common/Protocol.hpp"
#include "common/PacketBuffer.hpp"

/**
 * @class FeatureExtractor
 * @brief Класс для формирования векторов признаков из сетевого трафика.
 */
class FeatureExtractor {
public:
  FeatureExtractor();

  // Загрузка параметров масштабировщика из JSON-файла
  bool loadScalerParams(const std::string& jsonPath);

  // Обработка пакета в рамках текущего окна
  void processPacket(PacketBuffer* pktBuf);
  void processPacket(pcpp::RawPacket& rawPacket); // Обработка сырого пакета

  // Вычисление признаков для всех активных потоков в окне (пакетная обработка)
  std::vector<std::vector<double>> computeFeaturesBatch();

  // Получение нормализованных признаков для всех потоков
  std::vector<std::vector<double>> computeNormalizedFeaturesBatch();

  // Получение нормализованных признаков (агрегированных) для окна
  std::vector<double> computeNormalizedFeatures();

  // Заполнение структуры телеметрии для визуализации в GUI
  void fillTelemetry(DetectionResult& result) const;

  // Сброс состояния для начала нового окна агрегации
  void reset();

  // Установка параметров масштабировщика для конкретной модели
  void setModelScaler(const std::string& modelName);

  // Геттеры состояния
  int featureCount() const { return scaler_.features.empty() ? 16 : (int)scaler_.features.size(); }
  size_t activeFlowsCount() const { return activeFlows_.size(); }

private:
  // Параметры нормализации z-score
  struct ScalerParams {
    std::vector<std::string> features; // Список имен признаков
    std::vector<double> mean;          // Средние значения (μ)
    std::vector<double> scale;         // Стандартные отклонения (σ)
    bool useLog1p = false;             // Флаг логарифмирования
  };

  ScalerParams scaler_;                // Текущие параметры скейлера
  bool scalerLoaded_ = false;          // Флаг готовности скейлера
  std::string currentScalerPath_;      // Путь к загруженному скейлеру

  // Состояние окна агрегации
  std::chrono::steady_clock::time_point windowStart_; // Время начала окна
  bool windowStarted_ = false;                        // Флаг активности окна

  // Идентификатор сетевого потока (5-tuple)
  struct FlowKey {
    std::string ip1, ip2; // IP адреса (лексикографически отсортированные)
    uint16_t port1, port2; // Порты
    uint8_t proto;        // Протокол

    // Оператор сравнения для использования в std::map
    bool operator<(const FlowKey& o) const {
      return std::tie(ip1, ip2, port1, port2, proto) <
             std::tie(o.ip1, o.ip2, o.port1, o.port2, o.proto);
    }
  };

  // Статистика по конкретному потоку
  struct FlowStats {
    std::string initiatorIp; // IP адрес инициатора потока
    std::chrono::steady_clock::time_point firstPacketTime; // Время первого пакета
    std::chrono::steady_clock::time_point lastPacketTime;  // Время последнего пакета
    uint64_t fwdPackets = 0; // Пакетов в прямом направлении
    uint64_t bwdPackets = 0; // Пакетов в обратном направлении
    uint64_t fwdBytes = 0;   // Байтов в прямом направлении
    uint64_t bwdBytes = 0;   // Байтов в обратном направлении
    
    // Расширенные признаки TCP
    uint64_t synPackets = 0; // Кол-во SYN флагов
    uint64_t ackPackets = 0; // Кол-во ACK флагов
    uint64_t finPackets = 0; // Кол-во FIN флагов
    uint64_t rstPackets = 0; // Кол-во RST флагов
    uint64_t pshPackets = 0; // Кол-во PSH флагов
    uint64_t urgPackets = 0; // Кол-во URG флагов
    uint64_t tcpWindowSizeTotal = 0; // Суммарный размер окна TCP
    
    // Данные для расчета энтропии полезной нагрузки
    std::vector<uint64_t> byteCounts; // Частотный словарь байтов
    uint64_t totalPayloadBytes = 0;  // Общий размер нагрузки

    FlowStats() : byteCounts(256, 0) {}
  };

  std::map<FlowKey, FlowStats> activeFlows_; // Карта активных потоков

  // Глобальные счетчики окна (для телеметрии)
  uint64_t totalPackets_  = 0; // Всего пакетов
  uint64_t tcpPackets_    = 0; // TCP пакетов
  uint64_t udpPackets_    = 0; // UDP пакетов
  uint64_t icmpPackets_   = 0; // ICMP пакетов
  uint64_t otherPackets_  = 0; // Прочие протоколы
  uint64_t synPackets_    = 0; // Глобальный счетчик SYN
  uint64_t finPackets_    = 0; // Глобальный счетчик FIN
  uint64_t rstPackets_    = 0; // Глобальный счетчик RST
  uint64_t totalBytes_    = 0; // Общий объем трафика (байт)

  // Данные для анализа "Top Talkers"
  std::map<std::string, uint64_t> srcIpCounts_; // Частота IP источников
  std::map<uint16_t, uint64_t>    dstPortCounts_; // Частота портов назначения
  std::map<std::string, uint64_t> targetCounts_; // Частота целей (IP:Port)
  std::set<std::string>           uniqueSources_; // Набор уникальных источников
  std::vector<int>                sizeHistogram_; // Гистограмма размеров (5 бинов)
};
