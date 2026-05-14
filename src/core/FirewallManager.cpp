#include "core/FirewallManager.hpp"
#include "common/AppLogger.hpp"
#include <mutex>
#include <QHostAddress>
#include <QProcess>
#include <QStringList>

static std::mutex g_firewallMutex;

void FirewallManager::blockIp(const std::string& ip) {
    // 1. Валидация IP (из входящей ветки)
    QHostAddress addr(QString::fromStdString(ip));
    if (addr.isNull()) {
        AppLogger::get()->error("[SECURITY] Invalid IP format for firewall rule: '{}'. Operation aborted.", ip);
        return;
    }

    std::lock_guard<std::mutex> lock(g_firewallMutex);
    if (activeBlockedIps_.count(ip)) return;

#ifdef _WIN32
    AppLogger::get()->warn("Active Mitigation: Executing firewall block for IP {}", ip);
    
    // 2. Безопасный асинхронный запуск (из текущей ветки main)
    QString program = "netsh";
    QStringList arguments;
    arguments << "advfirewall" << "firewall" << "add" << "rule"
              << "name=DDoS_Block_" + addr.toString()
              << "dir=in" << "action=block"
              << "remoteip=" + addr.toString();

    QProcess::startDetached(program, arguments);
    
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

#ifdef _WIN32
    AppLogger::get()->info("Active Mitigation: Removing firewall block for IP {}", ip);

    QString program = "netsh";
    QStringList arguments;
    arguments << "advfirewall" << "firewall" << "delete" << "rule"
              << "name=DDoS_Block_" + addr.toString();

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
    // Delete any rules starting with "DDoS_Block_" using wildcard
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
