#include "network/TrafficMonitor.hpp"
#include "common/AppLogger.hpp"

#include <PcapLiveDeviceList.h>
#include <PcapFileDevice.h>
#include <chrono>
#include <thread>

TrafficMonitor::TrafficMonitor() {}

TrafficMonitor::~TrafficMonitor() {
    stopCapture();
}

std::vector<std::pair<std::string, std::string>> TrafficMonitor::listInterfaces() {
    std::vector<std::pair<std::string, std::string>> result;
    auto& devList = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList();
    AppLogger::get()->info("TrafficMonitor: found {} raw devices.", devList.size());
    if (devList.empty()) {
        AppLogger::get()->warn("If 0 devices is unexpected, ensure you are running as Administrator and Npcap is installed.");
    }
    for (auto* dev : devList) {
        std::string name = dev->getName();
        std::string desc = dev->getDesc();
        std::string ipv4 = dev->getIPv4Address().toString();
        std::string display = desc.empty() ? name : desc;
        if (!ipv4.empty() && ipv4 != "0.0.0.0")
            display += " [" + ipv4 + "]";
        result.emplace_back(name, display);
    }
    return result;
}

static std::string cleanString(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (std::isalnum((unsigned char)c)) out += (char)std::tolower((unsigned char)c);
    }
    return out;
}

bool TrafficMonitor::startCapture(const std::string& interfaceName) {
    auto devList = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList();
    device_ = nullptr;

    if (devList.empty()) {
        AppLogger::get()->error("TrafficMonitor: 0 raw devices found. Ensure you are running as Administrator!");
        return false;
    }

    std::string query = cleanString(interfaceName);
    AppLogger::get()->info("TrafficMonitor: searching for interface '{}' (query: '{}')", interfaceName, query);

    // 1. Пытаемся найти точное совпадение по имени или IP
    for (auto* dev : devList) {
        if (dev->getName() == interfaceName || dev->getIPv4Address().toString() == interfaceName) {
            device_ = dev;
            break;
        }
    }

    // 2. Ищем по очищенному описанию, приоритизируем физические устройства
    if (!device_) {
        pcpp::PcapLiveDevice* candidate = nullptr;
        for (auto* dev : devList) {
            std::string desc = cleanString(dev->getDesc());
            if (desc.find(query) != std::string::npos) {
                // Если это не виртуальный адаптер, берем сразу
                bool isVirtual = (desc.find("microsoft") != std::string::npos || 
                                  desc.find("virtual") != std::string::npos ||
                                  desc.find("hyperv") != std::string::npos ||
                                  desc.find("miniport") != std::string::npos);
                
                if (!isVirtual) {
                    device_ = dev;
                    break;
                } else if (!candidate) {
                    candidate = dev; // Запоминаем как запасной вариант
                }
            }
        }
        if (!device_) device_ = candidate;
    }

    if (!device_) {
        AppLogger::get()->error("TrafficMonitor: interface '{}' not found.", interfaceName);
        return false;
    }

    AppLogger::get()->info("TrafficMonitor: selected device '{}' ({})", device_->getName(), device_->getDesc());

    if (!device_->open()) {
        AppLogger::get()->error("TrafficMonitor: failed to open device '{}'.", interfaceName);
        return false;
    }

    if (!bpfFilter_.empty()) {
        if (!device_->setFilter(bpfFilter_)) {
            AppLogger::get()->warn("TrafficMonitor: failed to set BPF filter: {}", bpfFilter_);
        }
    }

    capturing_ = true;
    device_->startCapture(onPacketArrived, this);
    AppLogger::get()->info("TrafficMonitor: started capture on '{}'.", interfaceName);
    return true;
}

void TrafficMonitor::stopCapture() {
    if (capturing_ && device_) {
        device_->stopCapture();
        device_->close();
        capturing_ = false;
        AppLogger::get()->info("TrafficMonitor: capture stopped.");
    }
}

bool TrafficMonitor::isCapturing() const { return capturing_; }

bool TrafficMonitor::replayPcap(const std::string& filePath) {
    pcpp::IFileReaderDevice* reader = pcpp::IFileReaderDevice::getReader(filePath);
    if (!reader || !reader->open()) {
        AppLogger::get()->error("TrafficMonitor: cannot open pcap file: {}", filePath);
        if (reader) delete reader;
        return false;
    }

    AppLogger::get()->info("TrafficMonitor: replaying pcap: {}", filePath);
    capturing_ = true;

    pcpp::RawPacket rawPacket;
    timespec firstPktTime = {0, 0};
    auto replayStart = std::chrono::steady_clock::now();
    bool firstPacket = true;

    while (reader->getNextPacket(rawPacket) && capturing_) {
        // Pace replay using pcap timestamps
        if (firstPacket) {
            firstPktTime = rawPacket.getPacketTimeStamp();
            firstPacket = false;
        } else {
            // Calculate time offset from first packet
            timespec pktTime = rawPacket.getPacketTimeStamp();
            double pktOffset = (pktTime.tv_sec - firstPktTime.tv_sec)
                + (pktTime.tv_nsec - firstPktTime.tv_nsec) / 1e9;

            // Calculate how long we've been replaying
            auto elapsed = std::chrono::steady_clock::now() - replayStart;
            double elapsedSec = std::chrono::duration<double>(elapsed).count();

            // Sleep if we're ahead of the pcap timeline
            double sleepSec = pktOffset - elapsedSec;
            if (sleepSec > 0.001 && sleepSec < 10.0) {
                std::this_thread::sleep_for(
                    std::chrono::microseconds((int64_t)(sleepSec * 1e6)));
            }
        }

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
    AppLogger::get()->info("TrafficMonitor: replay complete. Packets: {}", totalCaptured_.load());
    return true;
}

void TrafficMonitor::setBpfFilter(const std::string& filter) {
    bpfFilter_ = filter;
    if (device_ && device_->isOpened()) {
        device_->setFilter(filter);
    }
}

bool TrafficMonitor::dequeuePacket(PacketBuffer*& packet) {
    return packetQueue_.try_dequeue(packet);
}

bool TrafficMonitor::dequeuePacketTimed(PacketBuffer*& packet, std::chrono::microseconds timeout) {
    return packetQueue_.wait_dequeue_timed(packet, timeout);
}

void TrafficMonitor::recycleBuffer(PacketBuffer* packet) {
    if (packet) {
        bufferPool_.enqueue(packet);
    }
}

PacketBuffer* TrafficMonitor::getBuffer() {
    PacketBuffer* buf = nullptr;
    if (!bufferPool_.try_dequeue(buf)) {
        buf = new PacketBuffer();
    }
    return buf;
}

size_t TrafficMonitor::queueSize() const {
    return packetQueue_.size_approx();
}

void TrafficMonitor::onPacketArrived(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie) {
    auto* self = static_cast<TrafficMonitor*>(cookie);
    if (self->packetQueue_.size_approx() < MAX_QUEUE_SIZE) {
        PacketBuffer* buf = self->getBuffer();
        buf->assign(packet->getRawData(), packet->getRawDataLen(), std::chrono::system_clock::now(), packet->getLinkLayerType());
        self->packetQueue_.enqueue(buf);
        self->totalCaptured_++;
    } else {
        self->droppedPackets_++;
    }
}
