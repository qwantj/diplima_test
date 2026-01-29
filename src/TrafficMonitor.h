#pragma once

#include <string>

namespace pcpp {
class PcapLiveDevice;
}

class TrafficMonitor {
public:
  TrafficMonitor();
  ~TrafficMonitor();

  bool start(const std::string& interfaceName);
  void stop();
  bool isRunning() const;

private:
  pcpp::PcapLiveDevice* device_;
};
