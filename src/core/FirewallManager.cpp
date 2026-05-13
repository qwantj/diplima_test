#include "core/FirewallManager.hpp"
#include "common/AppLogger.hpp"
#include <cstdlib>
#include <thread>
#include <mutex>

#include <QProcess>
#include <QHostAddress>
#include <QStringList>

static std::mutex g_firewallMutex;

void FirewallManager::blockIp(const std::string& ip) {
    std::lock_guard<std::mutex> lock(g_firewallMutex);
    if (activeBlockedIps_.count(ip)) return;

    if (QHostAddress(QString::fromStdString(ip)).isNull()) {
        AppLogger::get()->error("[SECURITY] Invalid IP format for firewall rule: '{}'. Operation aborted.", ip);
        return;
    }

#ifdef _WIN32
    AppLogger::get()->warn("Active Mitigation: Executing firewall block for IP {}", ip);
    
    QString program = "netsh";
    QStringList arguments;
    arguments << "advfirewall" << "firewall" << "add" << "rule"
              << "name=DDoS_Block_" + QString::fromStdString(ip)
              << "dir=in" << "action=block"
              << "remoteip=" + QString::fromStdString(ip);

    QProcess::startDetached(program, arguments);
    
    activeBlockedIps_.insert(ip);
#else
    AppLogger::get()->warn("Active Mitigation: Blocking IP {} not implemented for non-Windows platforms", ip);
#endif
}

void FirewallManager::unblockIp(const std::string& ip) {
    std::lock_guard<std::mutex> lock(g_firewallMutex);
    if (!activeBlockedIps_.count(ip)) return;

    if (QHostAddress(QString::fromStdString(ip)).isNull()) {
        AppLogger::get()->error("[SECURITY] Invalid IP format for firewall rule: '{}'. Operation aborted.", ip);
        return;
    }

#ifdef _WIN32
    AppLogger::get()->info("Active Mitigation: Removing firewall block for IP {}", ip);

    QString program = "netsh";
    QStringList arguments;
    arguments << "advfirewall" << "firewall" << "delete" << "rule"
              << "name=DDoS_Block_" + QString::fromStdString(ip);

    QProcess::startDetached(program, arguments);
#endif
    activeBlockedIps_.erase(ip);
}

void FirewallManager::unblockAllIps() {
    std::lock_guard<std::mutex> lock(g_firewallMutex);
#ifdef _WIN32
    for (const auto& ip : activeBlockedIps_) {
        AppLogger::get()->info("Active Mitigation: Removing firewall block for IP {}", ip);

        QString program = "netsh";
        QStringList arguments;
        arguments << "advfirewall" << "firewall" << "delete" << "rule"
                  << "name=DDoS_Block_" + QString::fromStdString(ip);

        QProcess::startDetached(program, arguments);
    }
#endif
    activeBlockedIps_.clear();
}

void FirewallManager::clearAllRules() {
    std::lock_guard<std::mutex> lock(g_firewallMutex);
#ifdef _WIN32
    // Delete any rules starting with "DDoS_Block_" just in case there are leftovers
    // We can just execute a wildcard delete safely through QProcess
    QString program = "netsh";
    QStringList arguments;
    arguments << "advfirewall" << "firewall" << "delete" << "rule" << "name=DDoS_Block_*";
    QProcess::startDetached(program, arguments);
#endif
    activeBlockedIps_.clear();
}

std::vector<std::string> FirewallManager::getBlockedIps() const {
    std::lock_guard<std::mutex> lock(g_firewallMutex);
    return std::vector<std::string>(activeBlockedIps_.begin(), activeBlockedIps_.end());
}