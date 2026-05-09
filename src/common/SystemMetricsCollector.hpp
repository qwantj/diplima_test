#pragma once

#include <cstdint>

struct SystemMetrics {
    double cpuUsagePercent  = 0.0;
    double ramUsagePercent  = 0.0;
    uint64_t ramUsedBytes   = 0;
    uint64_t ramTotalBytes  = 0;
};

class SystemMetricsCollector {
public:
    SystemMetricsCollector();
    SystemMetrics collect();

private:
#ifdef _WIN32
    uint64_t prevIdleTime_   = 0;
    uint64_t prevKernelTime_ = 0;
    uint64_t prevUserTime_   = 0;
    bool     initialized_    = false;
#endif
};
