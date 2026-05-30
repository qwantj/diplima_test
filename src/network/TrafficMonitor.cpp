/**
 * @file TrafficMonitor.cpp
 * @brief Модуль захвата сетевых пакетов (реализация).
 *
 * Назначение: Реализация методов класса TrafficMonitor для захвата и обработки трафика.
 * Входные данные: Пакеты с сетевого интерфейса или из pcap-файла.
 * Результаты: Заполнение очереди packetQueue_ объектами PacketBuffer.
 * Метод решения: Использование PcapPlusPlus для взаимодействия с драйвером Npcap.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#include "network/TrafficMonitor.hpp"
#include "common/AppLogger.hpp"

#include <PcapLiveDeviceList.h>
#include <PcapFileDevice.h>
#include <chrono>
#include <thread>

// Конструктор класса
TrafficMonitor::TrafficMonitor() {}

// Деструктор класса
TrafficMonitor::~TrafficMonitor() {
  stopCapture(); // Остановка захвата при удалении объекта
}

/**
 * @brief Формирование списка сетевых адаптеров.
 * @return Вектор пар (системное имя, дружественное описание).
 */
std::vector<std::pair<std::string, std::string>> TrafficMonitor::listInterfaces() {
  std::vector<std::pair<std::string, std::string>> result;
  auto& devList = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList();
  
  AppLogger::get()->info("TrafficMonitor: найдено {} сырых устройств.", devList.size());
  
  if (devList.empty()) {
    AppLogger::get()->warn("Если 0 устройств — это неожиданно, проверьте права Администратора и наличие Npcap.");
  }

  // Перебор всех найденных устройств
  for (auto* dev : devList) {
    std::string name = dev->getName();
    std::string desc = dev->getDesc();
    std::string ipv4 = dev->getIPv4Address().toString();
    std::string display = desc.empty() ? name : desc;
    
    // Добавление IP-адреса к описанию для удобства выбора
    if (!ipv4.empty() && ipv4 != "0.0.0.0")
      display += " [" + ipv4 + "]";
    
    result.emplace_back(name, display);
  }
  return result;
}

// Вспомогательная функция для очистки строки от спецсимволов при поиске
static std::string cleanString(const std::string& s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    if (std::isalnum((unsigned char)c)) out += (char)std::tolower((unsigned char)c);
  }
  return out;
}

/**
 * @brief Запуск живого захвата пакетов.
 * @param interfaceName Имя или IP интерфейса.
 * @return true в случае успеха.
 */
bool TrafficMonitor::startCapture(const std::string& interfaceName) {
  auto devList = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList();
  device_ = nullptr;

  if (devList.empty()) {
    AppLogger::get()->error("TrafficMonitor: 0 устройств найдено. Запустите от имени Администратора!");
    return false;
  }

  std::string query = cleanString(interfaceName);
  AppLogger::get()->info("TrafficMonitor: поиск интерфейса '{}' (запрос: '{}')", interfaceName, query);

  // 1. Поиск по точному совпадению имени или IP
  for (auto* dev : devList) {
    if (dev->getName() == interfaceName || dev->getIPv4Address().toString() == interfaceName) {
      device_ = dev;
      break;
    }
  }

  // 2. Нечеткий поиск по описанию, приоритет физическим устройствам
  if (!device_) {
    pcpp::PcapLiveDevice* candidate = nullptr;
    for (auto* dev : devList) {
      std::string desc = cleanString(dev->getDesc());
      if (desc.find(query) != std::string::npos) {
        // Проверка на виртуальные адаптеры
        bool isVirtual = (desc.find("microsoft") != std::string::npos || 
                  desc.find("virtual") != std::string::npos ||
                  desc.find("hyperv") != std::string::npos ||
                  desc.find("miniport") != std::string::npos);
        
        if (!isVirtual) {
          device_ = dev;
          break;
        } else if (!candidate) {
          candidate = dev;
        }
      }
    }
    if (!device_) device_ = candidate;
  }

  if (!device_) {
    AppLogger::get()->error("TrafficMonitor: интерфейс '{}' не найден.", interfaceName);
    return false;
  }

  AppLogger::get()->info("TrafficMonitor: выбрано устройство '{}' ({})", device_->getName(), device_->getDesc());

  // Открытие устройства для захвата
  if (!device_->open()) {
    AppLogger::get()->error("TrafficMonitor: ошибка открытия устройства '{}'.", interfaceName);
    return false;
  }

  // Установка BPF-фильтра, если он задан
  if (!bpfFilter_.empty()) {
    if (!device_->setFilter(bpfFilter_)) {
      AppLogger::get()->warn("TrafficMonitor: ошибка установки фильтра BPF: {}", bpfFilter_);
    }
  }

  capturing_ = true;
  // Запуск потока захвата драйвером
  device_->startCapture(onPacketArrived, this);
  AppLogger::get()->info("TrafficMonitor: захват запущен на '{}'.", interfaceName);
  return true;
}

// Остановка процесса захвата
void TrafficMonitor::stopCapture() {
  if (capturing_ && device_) {
    device_->stopCapture();
    device_->close();
    capturing_ = false;
    AppLogger::get()->info("TrafficMonitor: захват остановлен.");
  }
}

// Статус захвата
bool TrafficMonitor::isCapturing() const { return capturing_; }

/**
 * @brief Воспроизведение трафика из pcap-файла.
 * @param filePath Путь к файлу.
 * @return true в случае успешного завершения.
 */
bool TrafficMonitor::replayPcap(const std::string& filePath) {
  pcpp::IFileReaderDevice* reader = pcpp::IFileReaderDevice::getReader(filePath);
  if (!reader || !reader->open()) {
    AppLogger::get()->error("TrafficMonitor: не удалось открыть pcap-файл: {}", filePath);
    if (reader) delete reader;
    return false;
  }

  AppLogger::get()->info("TrafficMonitor: воспроизведение pcap: {}", filePath);
  capturing_ = true;

  pcpp::RawPacket rawPacket;
  timespec firstPktTime = {0, 0};
  auto replayStart = std::chrono::steady_clock::now();
  bool firstPacket = true;

  // Цикл чтения пакетов из файла
  while (reader->getNextPacket(rawPacket) && capturing_) {
    // Регулировка скорости воспроизведения согласно временным меткам в pcap
    if (firstPacket) {
      firstPktTime = rawPacket.getPacketTimeStamp();
      firstPacket = false;
    } else {
      timespec pktTime = rawPacket.getPacketTimeStamp();
      double pktOffset = (pktTime.tv_sec - firstPktTime.tv_sec)
        + (pktTime.tv_nsec - firstPktTime.tv_nsec) / 1e9;

      auto elapsed = std::chrono::steady_clock::now() - replayStart;
      double elapsedSec = std::chrono::duration<double>(elapsed).count();

      double sleepSec = pktOffset - elapsedSec;
      if (sleepSec > 0.001 && sleepSec < 10.0) {
        std::this_thread::sleep_for(
          std::chrono::microseconds((int64_t)(sleepSec * 1e6)));
      }
    }

    // Помещение пакета в очередь
    if (packetQueue_.size_approx() < MAX_QUEUE_SIZE) {
      PacketBuffer* buf = getBuffer();
      buf->assign(rawPacket.getRawData(), rawPacket.getRawDataLen(), std::chrono::system_clock::now(), rawPacket.getLinkLayerType());
      packetQueue_.enqueue(buf);
      totalCaptured_++;
    } else {
      droppedPackets_++;
    }
  }

  capturing_ = false;
  reader->close();
  delete reader;
  AppLogger::get()->info("TrafficMonitor: воспроизведение завершено. Пакетов: {}", totalCaptured_.load());
  return true;
}

// Установка фильтра BPF
void TrafficMonitor::setBpfFilter(const std::string& filter) {
  bpfFilter_ = filter;
  if (device_ && device_->isOpened()) {
    device_->setFilter(filter);
  }
}

// Извлечение пакета из очереди (неблокирующее)
bool TrafficMonitor::dequeuePacket(PacketBuffer*& packet) {
  return packetQueue_.try_dequeue(packet);
}

// Извлечение пакета с таймаутом
bool TrafficMonitor::dequeuePacketTimed(PacketBuffer*& packet, std::chrono::microseconds timeout) {
  return packetQueue_.wait_dequeue_timed(packet, timeout);
}

// Возврат буфера в пул
void TrafficMonitor::recycleBuffer(PacketBuffer* packet) {
  if (packet) {
    bufferPool_.enqueue(packet);
  }
}

// Получение свободного буфера (из пула или создание нового)
PacketBuffer* TrafficMonitor::getBuffer() {
  PacketBuffer* buf = nullptr;
  if (!bufferPool_.try_dequeue(buf)) {
    buf = new PacketBuffer();
  }
  return buf;
}

// Текущий размер очереди
size_t TrafficMonitor::queueSize() const {
  return packetQueue_.size_approx();
}

/**
 * @brief Коллбэк-функция, вызываемая драйвером при поступлении пакета.
 * @param packet Сырой пакет.
 * @param dev Устройство.
 * @param cookie Указатель на объект TrafficMonitor.
 */
void TrafficMonitor::onPacketArrived(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie) {
  auto* self = static_cast<TrafficMonitor*>(cookie);
  
  // Проверка переполнения очереди
  if (self->packetQueue_.size_approx() < MAX_QUEUE_SIZE) {
    PacketBuffer* buf = self->getBuffer();
    buf->assign(packet->getRawData(), packet->getRawDataLen(), std::chrono::system_clock::now(), packet->getLinkLayerType());
    self->packetQueue_.enqueue(buf);
    self->totalCaptured_++;
  } else {
    self->droppedPackets_++; // Счетчик отброшенных пакетов
  }
}
