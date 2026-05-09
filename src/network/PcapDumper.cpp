#include "network/PcapDumper.hpp"
#include "common/AppLogger.hpp"

#include <QDir>
#include <QDateTime>
#include <filesystem>

PcapDumper::PcapDumper() {}

PcapDumper::~PcapDumper() { close(); }

bool PcapDumper::startSession(const std::string& baseDir, const std::string& sessionName) {
    close();

    std::string dirPath = baseDir + "/" + sessionName;
    std::filesystem::create_directories(dirPath);

    currentPath_ = dirPath + "/capture.pcap";
    writer_ = new pcpp::PcapFileWriterDevice(currentPath_);

    if (!writer_->open()) {
        AppLogger::get()->error("PcapDumper: failed to open {}", currentPath_);
        delete writer_;
        writer_ = nullptr;
        return false;
    }

    AppLogger::get()->info("PcapDumper: started session at {}", currentPath_);
    return true;
}

void PcapDumper::writePacket(const pcpp::RawPacket& packet, int label) {
    if (writer_)
        writer_->writePacket(packet);
}

void PcapDumper::close() {
    if (writer_) {
        writer_->close();
        delete writer_;
        writer_ = nullptr;
        AppLogger::get()->info("PcapDumper: closed.");
    }
}
