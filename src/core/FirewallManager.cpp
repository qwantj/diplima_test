#include "core/FirewallManager.hpp"
#include "common/AppLogger.hpp"
#include <cstdlib>
#include <thread>
#include <mutex>

static std::mutex g_firewallMutex;

void FirewallManager::blockIp(const std::string& ip) {
    std::lock_guard<std::mutex> lock(g_firewallMutex);
    if (activeBlockedIps_.count(ip)) return;

#ifdef _WIN32
    std::string ruleName = "DDoS_Block_" + ip;
    std::string cmd = "netsh advfirewall firewall add rule name=\"" + ruleName + 
                      "\" dir=in action=block remoteip=" + ip;
    
    AppLogger::get()->warn("Active Mitigation: Executing firewall block for IP {}", ip);
    
    // We execute this asynchronously so it doesn't block the inference loop
    std::thread([cmd, ip, this]() {
        int result = std::system(cmd.c_str());
        if (result == 0) {
            AppLogger::get()->info("Active Mitigation: Successfully blocked IP {}", ip);
        } else {
            AppLogger::get()->error("Active Mitigation: Failed to block IP {} (Are you running as Administrator?)", ip);
        }
    }).detach();
    
    activeBlockedIps_.insert(ip);
#else
    AppLogger::get()->warn("Active Mitigation: Blocking IP {} not implemented for non-Windows platforms", ip);
#endif
}

void FirewallManager::unblockIp(const std::string& ip) {
    std::lock_guard<std::mutex> lock(g_firewallMutex);
    if (!activeBlockedIps_.count(ip)) return;

#ifdef _WIN32
    std::string ruleName = "DDoS_Block_" + ip;
    std::string cmd = "netsh advfirewall firewall delete rule name=\"" + ruleName + "\"";
    
    AppLogger::get()->info("Active Mitigation: Removing firewall block for IP {}", ip);
    std::thread([cmd]() {
        std::system(cmd.c_str());
    }).detach();
#endif
    activeBlockedIps_.erase(ip);
}

void FirewallManager::unblockAllIps() {
    std::lock_guard<std::mutex> lock(g_firewallMutex);
#ifdef _WIN32
    for (const auto& ip : activeBlockedIps_) {
        std::string ruleName = "DDoS_Block_" + ip;
        std::string cmd = "netsh advfirewall firewall delete rule name=\"" + ruleName + "\"";
        
        AppLogger::get()->info("Active Mitigation: Removing firewall block for IP {}", ip);
        std::thread([cmd]() {
            std::system(cmd.c_str());
        }).detach();
    }
#endif
    activeBlockedIps_.clear();
}

void FirewallManager::clearAllRules() {
    std::lock_guard<std::mutex> lock(g_firewallMutex);
#ifdef _WIN32
    // Delete any rules starting with "DDoS_Block_" just in case there are leftovers
    std::string cmd = "netsh advfirewall firewall delete rule name=all | findstr DDoS_Block_ && netsh advfirewall firewall delete rule name=\"DDoS_Block_*\"";
    std::thread([cmd]() {
        std::system("netsh advfirewall firewall delete rule name=all | findstr DDoS_Block_ >nul 2>&1"); // Check if any exist to avoid errors in console
        // We can just execute a wildcard delete
        std::system("netsh advfirewall firewall delete rule name=\"DDoS_Block_*\" >nul 2>&1");
    }).detach();
#endif
    activeBlockedIps_.clear();
}

std::vector<std::string> FirewallManager::getBlockedIps() const {
    std::lock_guard<std::mutex> lock(g_firewallMutex);
    return std::vector<std::string>(activeBlockedIps_.begin(), activeBlockedIps_.end());
}