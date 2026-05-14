#include "core/FirewallManager.hpp"
#include "common/AppLogger.hpp"
#include <cstdlib>
#include <thread>
#include <mutex>
#include <QHostAddress>
#include <QProcess>

#include <QProcess>
#include <QHostAddress>
#include <QStringList>

static std::mutex g_firewallMutex;

void FirewallManager::blockIp(const std::string& ip) {
    QHostAddress addr(QString::fromStdString(ip));
    if (addr.isNull()) {
        AppLogger::get()->error("[SECURITY] Invalid IP address blocked: {}", ip);
        return;
    }

    std::lock_guard<std::mutex> lock(g_firewallMutex);
    if (activeBlockedIps_.count(ip)) return;

    if (QHostAddress(QString::fromStdString(ip)).isNull()) {
        AppLogger::get()->error("[SECURITY] Invalid IP format for firewall rule: '{}'. Operation aborted.", ip);
        return;
    }

#ifdef _WIN32
    AppLogger::get()->warn("Active Mitigation: Executing firewall block for IP {}", ip);
    
    // We execute this asynchronously so it doesn't block the inference loop
    std::thread([ip]() {
        QString program = "netsh";
        QStringList arguments;
        arguments << "advfirewall" << "firewall" << "add" << "rule"
                  << "name=DDoS_Block_" + QString::fromStdString(ip)
                  << "dir=in" << "action=block" << "remoteip=" + QString::fromStdString(ip);

        int result = QProcess::execute(program, arguments);
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
    QHostAddress addr(QString::fromStdString(ip));
    if (addr.isNull()) {
        AppLogger::get()->error("[SECURITY] Invalid IP address for unblock: {}", ip);
        return;
    }

    std::lock_guard<std::mutex> lock(g_firewallMutex);
    if (!activeBlockedIps_.count(ip)) return;

    if (QHostAddress(QString::fromStdString(ip)).isNull()) {
        AppLogger::get()->error("[SECURITY] Invalid IP format for firewall rule: '{}'. Operation aborted.", ip);
        return;
    }

#ifdef _WIN32
    AppLogger::get()->info("Active Mitigation: Removing firewall block for IP {}", ip);
    std::thread([ip]() {
        QString program = "netsh";
        QStringList arguments;
        arguments << "advfirewall" << "firewall" << "delete" << "rule"
                  << "name=DDoS_Block_" + QString::fromStdString(ip);
        QProcess::execute(program, arguments);
    }).detach();
#endif
    activeBlockedIps_.erase(ip);
}

void FirewallManager::unblockAllIps() {
    std::lock_guard<std::mutex> lock(g_firewallMutex);
#ifdef _WIN32
    for (const auto& ip : activeBlockedIps_) {
        AppLogger::get()->info("Active Mitigation: Removing firewall block for IP {}", ip);
        std::thread([ip]() {
            QString program = "netsh";
            QStringList arguments;
            arguments << "advfirewall" << "firewall" << "delete" << "rule"
                      << "name=DDoS_Block_" + QString::fromStdString(ip);
            QProcess::execute(program, arguments);
        }).detach();
    }
#endif
    activeBlockedIps_.clear();
}

void FirewallManager::clearAllRules() {
    std::lock_guard<std::mutex> lock(g_firewallMutex);
#ifdef _WIN32
    std::thread([]() {
        QString program = "netsh";
        QStringList arguments;
        arguments << "advfirewall" << "firewall" << "delete" << "rule"
                  << "name=DDoS_Block_*";
        QProcess::execute(program, arguments);
    }).detach();
#endif
    activeBlockedIps_.clear();
}

std::vector<std::string> FirewallManager::getBlockedIps() const {
    std::lock_guard<std::mutex> lock(g_firewallMutex);
    return std::vector<std::string>(activeBlockedIps_.begin(), activeBlockedIps_.end());
}
