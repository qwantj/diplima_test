#include "TrafficMonitor.h"

#include <PcapLiveDevice.h>
#include <PcapLiveDeviceList.h>

TrafficMonitor::TrafficMonitor()
  : device_(nullptr) {}

TrafficMonitor::~TrafficMonitor() {
  stop();
}

bool TrafficMonitor::start(const std::string& interfaceName) {
  if (interfaceName.empty()) {
    return false;
  }

  if (device_ && device_->isOpened()) {
    return false;
  }

  device_ = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(interfaceName);
  if (device_ == nullptr) {
    return false;
  }

  if (!device_->open()) {
    device_ = nullptr;
    return false;
  }

  return true;
}

void TrafficMonitor::stop() {
  if (device_ && device_->isOpened()) {
    device_->close();
  }
  device_ = nullptr;
}

bool TrafficMonitor::isRunning() const {
  return device_ != nullptr && device_->isOpened();
}
