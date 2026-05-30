/**
 * @file DetectionEngine.hpp
 * @brief Ядро системы обнаружения атак (заголовок).
 *
 * Назначение: Координация работы модулей захвата, извлечения признаков и инференса.
 * Входные данные: Параметры конфигурации, сетевой трафик.
 * Результаты: События обнаружения и управление механизмами защиты.
 * Метод решения: Реализация главного цикла мониторинга в отдельном потоке, отслеживание состояний инцидента.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#pragma once // Директива защиты от повторного включения

#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <memory>
#include <deque>

#include "network/TrafficMonitor.hpp"
#include "network/FeatureExtractor.hpp"
#include "network/PcapDumper.hpp"
#include "ml/ModelInferencer.hpp"
#include "common/Protocol.hpp"

#include <pcapplusplus/RawPacket.h>

/**
 * @class DetectionEngine
 * @brief Центральный оркестратор процесса обнаружения DDoS-атак.
 */
class DetectionEngine {
public:
  DetectionEngine();
  ~DetectionEngine();

  // Типы обратных вызовов для передачи результатов
  using ResultCallback = std::function<void(const DetectionResult&)>;
  using IncidentCallback = std::function<void(const QDateTime& startTime, float duration, const std::string& attackerIp, float ppsMax, const std::string& typeLabel, float confidence)>;
  
  void setResultCallback(ResultCallback cb) { resultCallback_ = cb; }
  void setIncidentCallback(IncidentCallback cb) { incidentCallback_ = cb; }

  // Инициализация подсистем (загрузка модели и скейлера)
  bool init(const std::string& modelPath,
            const std::string& scalerPath = "",
            const std::string& ep = "cpu");

  // Запуск мониторинга в реальном времени
  bool startLive(const std::string& interfaceName);

  // Запуск анализа из pcap-файла
  bool startReplay(const std::string& pcapPath);

  // Остановка всех процессов обнаружения
  void stop();
  bool isRunning() const { return running_; }

  // Установка BPF-фильтра для TrafficMonitor
  void setBpfFilter(const std::string& filter);

  // Горячая замена модели без перезапуска
  bool hotSwapModel(const std::string& modelPath, const std::string& ep = "cpu");

  // Управление дампом трафика (сохранение pcap)
  void enablePcapDump(const std::string& dir);
  void disablePcapDump();
  bool isDumpEnabled() const { return dumpEnabled_; }

  // Управление режимом активной защиты (блокировкой)
  void setMitigationEnabled(bool enabled);

  // Доступ к внутренним модулям
  TrafficMonitor& trafficMonitor() { return monitor_; }
  FeatureExtractor& featureExtractor() { return extractor_; }
  ModelInferencer& modelInferencer() { return inferencer_; }

  // Длительность окна агрегации по умолчанию (1 сек)
  static constexpr double INFERENCE_WINDOW_SEC = 1.0;

private:
  // Основной рабочий цикл потока обнаружения
  void inferenceLoop();
  
  // Обработка накопленных данных за окно
  void processWindow();
  
  // Обновление состояния конечного автомата инцидентов
  void updateIncidentState(const DetectionResult& result);

  // Составные части ядра
  TrafficMonitor   monitor_;    // Модуль захвата
  FeatureExtractor extractor_;  // Модуль признаков
  ModelInferencer  inferencer_; // Модуль инференса
  PcapDumper       dumper_;     // Модуль сохранения дампов

  ResultCallback resultCallback_;     // Коллбэк для live-обновлений
  IncidentCallback incidentCallback_; // Коллбэк для итогов инцидента
  
  std::atomic<bool> running_{false};         // Флаг работы цикла
  std::atomic<bool> isReplayMode_{false};    // Режим воспроизведения
  std::atomic<bool> replayFinished_{false};  // Завершение файла
  std::unique_ptr<std::thread> inferenceThread_; // Поток инференса

  std::string pcapDumpDir_; // Директория для дампов
  bool dumpEnabled_ = false; // Флаг активности дампа

  // Порог фонового шума для предотвращения ложных срабатываний
  static constexpr double NOISE_THRESHOLD_PPS = 50.0;

  // Состояние текущего инцидента
  bool isUnderAttack_ = false;          // Флаг состояния атаки
  bool isMitigationEnabled_ = false;    // Флаг активности блокировки
  QDateTime attackStartTime_;           // Время начала текущей атаки
  double maxPps_ = 0.0;                 // Пиковая интенсивность
  std::string attackType_;              // Метка типа атаки
  float maxConfidence_ = 0.0;           // Максимальная уверенность
  std::map<std::string, uint64_t> attackSrcIps_; // Счетчики по IP атакующих
};
