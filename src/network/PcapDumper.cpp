#include "network/PcapDumper.hpp"
#include "common/AppLogger.hpp"

#include <filesystem>

PcapDumper::PcapDumper() {}

PcapDumper::~PcapDumper() { close(); }

bool PcapDumper::startSession(const std::string& baseDir, const std::string& sessionName) {
    close();

    sessionDir_ = baseDir + "/" + sessionName;
    try {
        std::filesystem::create_directories(sessionDir_);
    } catch (const std::exception& e) {
        AppLogger::get()->error("PcapDumper: failed to create directory {}: {}", sessionDir_, e.what());
        return false;
    }

    benignWriter_ = new pcpp::PcapFileWriterDevice(sessionDir_ + "/benign.pcap");
    attackWriter_ = new pcpp::PcapFileWriterDevice(sessionDir_ + "/attack.pcap");

    if (!benignWriter_->open()) {
        AppLogger::get()->error("PcapDumper: failed to open benign.pcap");
        return false;
    }
    if (!attackWriter_->open()) {
        AppLogger::get()->error("PcapDumper: failed to open attack.pcap");
        return false;
    }

    active_ = true;
    AppLogger::get()->info("PcapDumper: session started in {}", sessionDir_);
    return true;
}

void PcapDumper::addPacket(const pcpp::RawPacket& packet) {
    if (!active_) return;
    windowBuffer_.push_back(packet);
}

void PcapDumper::commitWindow(int label) {
    if (!active_) {
        windowBuffer_.clear();
        return;
    }

    pcpp::PcapFileWriterDevice* target = (label == 1) ? attackWriter_ : benignWriter_;
    
    if (target) {
        for (const auto& pkt : windowBuffer_) {
            target->writePacket(pkt);
        }
    }
    
    windowBuffer_.clear();
}

void PcapDumper::close() {
    active_ = false;
    windowBuffer_.clear();

    if (benignWriter_) {
        benignWriter_->close();
        delete benignWriter_;
        benignWriter_ = nullptr;
    }
    if (attackWriter_) {
        attackWriter_->close();
        delete attackWriter_;
        attackWriter_ = nullptr;
    }
    AppLogger::get()->info("PcapDumper: closed session.");
}
