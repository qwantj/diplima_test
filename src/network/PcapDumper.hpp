#pragma once

#include <string>
#include <vector>
#include <pcapplusplus/PcapFileDevice.h>
#include <pcapplusplus/RawPacket.h>

class PcapDumper {
public:
    PcapDumper();
    ~PcapDumper();

    bool startSession(const std::string& baseDir, const std::string& sessionName);
    
    // Window-based dumping
    void addPacket(const pcpp::RawPacket& packet);
    void commitWindow(int label); // 0: Benign, 1: Attack
    
    void close();
    bool isOpen() const { return active_; }

private:
    bool active_ = false;
    std::string sessionDir_;
    
    std::vector<pcpp::RawPacket> windowBuffer_;
    
    pcpp::PcapFileWriterDevice* benignWriter_ = nullptr;
    pcpp::PcapFileWriterDevice* attackWriter_ = nullptr;
};
