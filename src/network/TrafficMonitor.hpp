#pragma once

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

class TrafficMonitor {
public:
    TrafficMonitor();
    ~TrafficMonitor();

    // List available interfaces
    static std::vector<std::pair<std::string, std::string>> listInterfaces();

    // Start live capture
    bool startCapture(const std::string& interfaceName);
    void stopCapture();
    bool isCapturing() const;

    // Replay pcap file
    bool replayPcap(const std::string& filePath);

    // BPF filter
    void setBpfFilter(const std::string& filter);

    // Drain captured packets
    bool dequeuePacket(PacketBuffer*& packet);
    bool dequeuePacketTimed(PacketBuffer*& packet, std::chrono::microseconds timeout);
    void recycleBuffer(PacketBuffer* packet);
    size_t queueSize() const;

    // Stats
    uint64_t totalCaptured() const { return totalCaptured_.load(); }
    uint64_t droppedPackets() const { return droppedPackets_.load(); }

    static constexpr size_t MAX_QUEUE_SIZE = 500000;

private:
    static void onPacketArrived(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie);
    PacketBuffer* getBuffer();

    pcpp::PcapLiveDevice* device_ = nullptr;
    moodycamel::BlockingConcurrentQueue<PacketBuffer*> packetQueue_;
    moodycamel::ConcurrentQueue<PacketBuffer*> bufferPool_;
    std::atomic<bool> capturing_{false};
    std::atomic<uint64_t> totalCaptured_{0};
    std::atomic<uint64_t> droppedPackets_{0};
    std::string bpfFilter_;
};
