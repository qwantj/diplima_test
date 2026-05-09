#pragma once

#include <string>
#include <pcapplusplus/PcapFileDevice.h>
#include <pcapplusplus/RawPacket.h>

class PcapDumper {
public:
    PcapDumper();
    ~PcapDumper();

    bool startSession(const std::string& baseDir, const std::string& sessionName);
    void writePacket(const pcpp::RawPacket& packet, int label);
    void close();
    bool isOpen() const { return writer_ != nullptr; }

private:
    pcpp::PcapFileWriterDevice* writer_ = nullptr;
    std::string currentPath_;
};
