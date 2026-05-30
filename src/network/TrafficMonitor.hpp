/**
 * @file TrafficMonitor.hpp
 * @brief Модуль захвата сетевых пакетов (заголовок).
 *
 * Назначение: Класс для низкоуровневого захвата сетевого трафика с использованием PcapPlusPlus и Npcap.
 * Входные данные: Имя сетевого интерфейса, BPF-фильтры.
 * Результаты: Очередь сырых пакетов для последующего анализа.
 * Метод решения: Асинхронный захват через callback-функцию драйвера с использованием lock-free очереди.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#pragma once // Директива защиты от повторного включения

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <memory>
#include <cstdint>

#include <PcapLiveDeviceList.h>
#include <PcapLiveDevice.h>
#include <PcapFileDevice.h>
#include <RawPacket.h>

#include "concurrentqueue.h"
#include "blockingconcurrentqueue.h"
#include "common/PacketBuffer.hpp"

/**
 * @class TrafficMonitor
 * @brief Класс для захвата и буферизации сетевого трафика.
 */
class TrafficMonitor {
public:
  TrafficMonitor();
  ~TrafficMonitor();

  // Получение списка доступных сетевых интерфейсов
  static std::vector<std::pair<std::string, std::string>> listInterfaces();

  // Запуск живого захвата пакетов
  bool startCapture(const std::string& interfaceName);
  
  // Остановка процесса захвата
  void stopCapture();
  
  // Проверка активности захвата
  bool isCapturing() const;

  // Воспроизведение pcap-файла
  bool replayPcap(const std::string& filePath);

  // Установка BPF-фильтра для захвата
  void setBpfFilter(const std::string& filter);

  // Извлечение захваченного пакета из очереди
  bool dequeuePacket(PacketBuffer*& packet);
  
  // Извлечение пакета с ограничением по времени
  bool dequeuePacketTimed(PacketBuffer*& packet, std::chrono::microseconds timeout);
  
  // Возврат буфера в пул для переиспользования
  void recycleBuffer(PacketBuffer* packet);
  
  // Текущее количество пакетов в очереди
  size_t queueSize() const;

  // Геттеры статистики
  uint64_t totalCaptured() const { return totalCaptured_.load(); }
  uint64_t droppedPackets() const { return droppedPackets_.load(); }

  // Константа максимального размера очереди
  static constexpr size_t MAX_QUEUE_SIZE = 500000;

private:
  // Статическая функция обратного вызова для драйвера
  static void onPacketArrived(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie);
  
  // Вспомогательный метод получения свободного буфера
  PacketBuffer* getBuffer();

  // Данные класса
  pcpp::PcapLiveDevice* device_ = nullptr; // Устройство захвата
  moodycamel::BlockingConcurrentQueue<PacketBuffer*> packetQueue_; // Основная очередь
  moodycamel::ConcurrentQueue<PacketBuffer*> bufferPool_; // Пул буферов
  
  std::atomic<bool> capturing_{false};    // Флаг работы
  std::atomic<uint64_t> totalCaptured_{0}; // Всего захвачено
  std::atomic<uint64_t> droppedPackets_{0}; // Отброшено пакетов
  std::string bpfFilter_;                 // Строка фильтра BPF
};
