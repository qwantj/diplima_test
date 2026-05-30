/**
 * @file DetectionEngine.cpp
 * @brief Ядро системы обнаружения атак (реализация).
 *
 * Назначение: Реализация логики управления конвейером обработки трафика и принятия решений.
 * Входные данные: Сырые пакеты из TrafficMonitor.
 * Результаты: Аналитические отчеты DetectionResult, управление FirewallManager.
 * Метод решения: Использование паттерна "Производитель-Потребитель" с временными окнами агрегации.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#include "core/DetectionEngine.hpp"
#include "common/AppLogger.hpp"
#include "core/FirewallManager.hpp"

#include <chrono>
#include <QDateTime>
#include <cstdlib>

// Конструктор
DetectionEngine::DetectionEngine() {}

// Деструктор: очистка правил фаервола при завершении
DetectionEngine::~DetectionEngine() {
  stop();
  FirewallManager::getInstance().clearAllRules();
}

/**
 * @brief Инициализация движка.
 * @param modelPath Путь к ONNX модели.
 * @param scalerPath Путь к JSON скейлеру.
 * @param ep Провайдер (cpu/dml).
 * @return true при успехе.
 */
bool DetectionEngine::init(const std::string& modelPath,
              const std::string& scalerPath,
              const std::string& ep) {
  if (!inferencer_.loadModel(modelPath, ep))
    return false;

  // Настройка масштабировщика признаков
  if (!scalerPath.empty()) {
    extractor_.loadScalerParams(scalerPath);
  } else {
    extractor_.setModelScaler(modelPath);
  }

  AppLogger::get()->info("DetectionEngine: инициализирован с моделью '{}', EP={}",
    inferencer_.modelName(), ep);
  return true;
}

// Запуск живого мониторинга на интерфейсе
bool DetectionEngine::startLive(const std::string& interfaceName) {
  if (running_) stop();

  if (!monitor_.startCapture(interfaceName))
    return false;

  isReplayMode_ = false;
  replayFinished_ = false;
  running_ = true;
  // Создание потока для цикла инференса
  inferenceThread_ = std::make_unique<std::thread>(&DetectionEngine::inferenceLoop, this);
  return true;
}

/**
 * @brief Запуск анализа pcap-файла.
 * @param pcapPath Путь к файлу.
 * @return true при успешном старте.
 */
bool DetectionEngine::startReplay(const std::string& pcapPath) {
  if (running_) stop();

  isReplayMode_ = true;
  replayFinished_ = false;
  running_ = true;
  inferenceThread_ = std::make_unique<std::thread>([this, pcapPath]() {
    // Поток воспроизведения (блокирующий)
    std::thread replayThread([this, pcapPath]() {
      monitor_.replayPcap(pcapPath);
    });

    // Основной цикл инференса работает параллельно с чтением файла
    inferenceLoop();

    if (replayThread.joinable())
      replayThread.join();
    
    AppLogger::get()->info("DetectionEngine: Поток воспроизведения завершен.");
  });

  return true;
}

// Остановка мониторинга и сопутствующих потоков
void DetectionEngine::stop() {
  running_ = false;
  isReplayMode_ = false;
  monitor_.stopCapture();

  if (inferenceThread_ && inferenceThread_->joinable())
    inferenceThread_->join();
  inferenceThread_.reset();

  if (dumpEnabled_)
    dumper_.close();
}

// Управление активной защитой
void DetectionEngine::setMitigationEnabled(bool enabled) {
  isMitigationEnabled_ = enabled;
  if (!enabled) {
    // Если защита отключена — снимаем все активные блокировки
    FirewallManager::getInstance().unblockAllIps();
  }
}

void DetectionEngine::setBpfFilter(const std::string& filter) {
  monitor_.setBpfFilter(filter);
}

// Горячая замена модели
bool DetectionEngine::hotSwapModel(const std::string& modelPath, const std::string& ep) {
  bool ok = inferencer_.hotSwapModel(modelPath, ep);
  if (ok) {
    extractor_.setModelScaler(modelPath);
  }
  return ok;
}

// Включение дампа трафика в файлы
void DetectionEngine::enablePcapDump(const std::string& dir) {
  pcapDumpDir_ = dir;
  dumpEnabled_ = true;

  auto sessionName = "session_" +
    QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss").toStdString();
  dumper_.startSession(dir, sessionName);
}

void DetectionEngine::disablePcapDump() {
  dumpEnabled_ = false;
  dumper_.close();
}

/**
 * @brief Основной рабочий цикл инференса.
 */
void DetectionEngine::inferenceLoop() {
  AppLogger::get()->info("DetectionEngine: цикл инференса запущен (окно={}с)",
    INFERENCE_WINDOW_SEC);

  // Ожидание старта захвата в режиме воспроизведения
  if (isReplayMode_) {
    int retries = 0;
    while (!monitor_.isCapturing() && retries++ < 10) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  }

  // Главный цикл (работает до вызова stop())
  while (running_) {
    auto windowStart = std::chrono::steady_clock::now();
    extractor_.reset();

    // Сбор пакетов в течение заданного окна (1 сек)
    while (running_) {
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration<double>(now - windowStart).count();
      if (elapsed >= INFERENCE_WINDOW_SEC)
        break;

      double remaining = INFERENCE_WINDOW_SEC - elapsed;
      auto timeout = std::chrono::microseconds(static_cast<long long>(remaining * 1e6));
      if (timeout.count() < 1000) timeout = std::chrono::microseconds(1000);

      PacketBuffer* pkt = nullptr;
      // Извлечение пакета из очереди TrafficMonitor
      if (monitor_.dequeuePacket(pkt)) {
        extractor_.processPacket(pkt);
        if (dumpEnabled_) {
          struct timeval tv = {0, 0};
          pcpp::RawPacket raw(pkt->data(), pkt->size(), tv, false);
          dumper_.addPacket(raw);
        }
        monitor_.recycleBuffer(pkt);
      } else {
        // Проверка окончания pcap-файла
        if (isReplayMode_ && !monitor_.isCapturing() && monitor_.queueSize() == 0) {
          replayFinished_ = true;
          goto final_window;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }

  final_window:
    if (!running_ && !replayFinished_) break;

    // Обработка накопленной за секунду статистики
    processWindow();
    
    if (replayFinished_) {
      running_ = false;
      break;
    }
  }

  AppLogger::get()->info("DetectionEngine: цикл инференса остановлен.");
}

/**
 * @brief Выполнение анализа окна трафика.
 */
void DetectionEngine::processWindow() {
  DetectionResult result;
  result.timestamp = QDateTime::currentDateTime();
  result.modelName = inferencer_.modelName();
  extractor_.fillTelemetry(result); // Сбор телеметрии

  result.droppedPackets = monitor_.droppedPackets();
  result.queueSize = monitor_.queueSize();
  result.blockedIps = FirewallManager::getInstance().getBlockedIps();

  // Пропуск, если пакетов нет
  if (result.totalPackets == 0) {
    if (dumpEnabled_) dumper_.commitWindow(0);
    return;
  }

  // Проверка на порог "шума" (незначительный трафик)
  if (result.pps < NOISE_THRESHOLD_PPS) {
    result.label = 0;
    result.confidence = 0.0f;
    result.features = std::vector<double>(extractor_.featureCount(), 0.0);
    
    if (dumpEnabled_) dumper_.commitWindow(0);
    if (resultCallback_) resultCallback_(result);
    return;
  }

  // Расчет нормализованных признаков
  auto features = extractor_.computeNormalizedFeatures();
  if (features.empty() || (int)features.size() != extractor_.featureCount())
    return;

  result.features = features;
  std::vector<float> featFloat(features.begin(), features.end());

  // Выполнение предсказания ML-моделью
  auto startInfer = std::chrono::steady_clock::now();
  auto [label, confidence] = inferencer_.predict(featFloat);
  auto endInfer = std::chrono::steady_clock::now();

  result.inferenceLatencyMs = std::chrono::duration<double, std::milli>(endInfer - startInfer).count();
  result.label = label;
  result.confidence = confidence;

  // Фиксация окна в дампе трафика
  if (dumpEnabled_) {
    dumper_.commitWindow(result.label);
  }

  // Обновление состояния инцидента и выполнение защитных действий
  updateIncidentState(result);

  // Передача результата подписчикам (GUI, БД)
  if (resultCallback_)
    resultCallback_(result);
}

/**
 * @brief Логика отслеживания переходов состояний Атака <-> Норма.
 */
void DetectionEngine::updateIncidentState(const DetectionResult& result) {
  bool currentlyAttacking = (result.label == 1);

  // Переход: Норма -> Атака
  if (!isUnderAttack_ && currentlyAttacking) {
    isUnderAttack_ = true;
    attackStartTime_ = result.timestamp;
    maxPps_ = result.pps;
    maxConfidence_ = result.confidence;
    attackType_ = "DDoS Attack";
    attackSrcIps_.clear();
    for (auto& [ip, count] : result.topTalkers) {
      attackSrcIps_[ip] += count;
    }
    AppLogger::get()->info("Incident Tracking: Начало атаки в {}", 
      attackStartTime_.toString("HH:mm:ss").toStdString());

    // Автоматическая блокировка top-агрессора
    if (isMitigationEnabled_ && !result.topTalkers.empty()) {
      FirewallManager::getInstance().blockIp(result.topTalkers[0].first);
    }
  }
  // Продолжение атаки
  else if (isUnderAttack_ && currentlyAttacking) {
    maxPps_ = std::max(maxPps_, result.pps);
    maxConfidence_ = std::max(maxConfidence_, result.confidence);
    for (auto& [ip, count] : result.topTalkers) {
      attackSrcIps_[ip] += count;
    }

    if (isMitigationEnabled_ && !result.topTalkers.empty()) {
      FirewallManager::getInstance().blockIp(result.topTalkers[0].first);
    }
  }
  // Переход: Атака -> Норма
  else if (isUnderAttack_ && !currentlyAttacking) {
    isUnderAttack_ = false;
    float duration = attackStartTime_.secsTo(result.timestamp);
    
    // Определение главного виновника атаки
    std::string topAttacker = "unknown";
    uint64_t maxPkts = 0;
    for (auto& [ip, count] : attackSrcIps_) {
      if (count > maxPkts) {
        maxPkts = count;
        topAttacker = ip;
      }
    }

    AppLogger::get()->info("Incident Tracking: Атака завершена. Длительность: {}с, Макс PPS: {:.0f}, Агрессор: {}", 
      duration, maxPps_, topAttacker);

    // Вызов коллбэка завершения инцидента
    if (incidentCallback_) {
      incidentCallback_(attackStartTime_, duration, topAttacker, 
               (float)maxPps_, attackType_, maxConfidence_);
    }
    
    // Снятие блокировок по окончании инцидента
    FirewallManager::getInstance().unblockAllIps();
  }
}
