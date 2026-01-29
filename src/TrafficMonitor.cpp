#include "TrafficMonitor.h"

#include <PcapLiveDevice.h>
#include <PcapLiveDeviceList.h>

TrafficMonitor::TrafficMonitor()
  : device_(nullptr), running_(false) {}

TrafficMonitor::~TrafficMonitor() {
  stop();
}

bool TrafficMonitor::start(const std::string& interfaceName) {
  if (running_) {
    return true;
  }

  device_ = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(interfaceName);
  if (device_ == nullptr) {
    return false;
  }

  if (!device_->open()) {
    device_ = nullptr;
    return false;
  }

  running_ = true;
  return true;
}

void TrafficMonitor::stop() {
  if (device_ && device_->isOpened()) {
    device_->close();
  }
  device_ = nullptr;
  running_ = false;
}

bool TrafficMonitor::isRunning() const {
  return running_;
}
