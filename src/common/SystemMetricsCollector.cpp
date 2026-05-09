#include "common/SystemMetricsCollector.hpp"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

SystemMetricsCollector::SystemMetricsCollector() {}

SystemMetrics SystemMetricsCollector::collect() {
    SystemMetrics m;

#ifdef _WIN32
    // RAM
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    if (GlobalMemoryStatusEx(&memInfo)) {
        m.ramTotalBytes = memInfo.ullTotalPhys;
        m.ramUsedBytes  = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
        m.ramUsagePercent = (double)m.ramUsedBytes / (double)m.ramTotalBytes * 100.0;
    }

    // CPU
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        auto toU64 = [](const FILETIME& ft) -> uint64_t {
            return ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
        };
        uint64_t idle   = toU64(idleTime);
        uint64_t kernel = toU64(kernelTime);
        uint64_t user   = toU64(userTime);

        if (initialized_) {
            uint64_t dIdle   = idle   - prevIdleTime_;
            uint64_t dKernel = kernel - prevKernelTime_;
            uint64_t dUser   = user   - prevUserTime_;
            uint64_t dTotal  = dKernel + dUser;
            if (dTotal > 0)
                m.cpuUsagePercent = (1.0 - (double)dIdle / (double)dTotal) * 100.0;
        }

        prevIdleTime_   = idle;
        prevKernelTime_ = kernel;
        prevUserTime_   = user;
        initialized_    = true;
    }
#endif

    return m;
}
