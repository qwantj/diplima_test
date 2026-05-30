/**
 * @file FirewallManager.cpp
 * @brief Модуль управления межсетевым экраном (реализация).
 *
 * Назначение: Реализация методов блокировки и разблокировки IP-адресов нарушителей.
 * Входные данные: IP-адреса.
 * Результаты: Вызов команд netsh для изменения правил фаервола.
 * Метод решения: Формирование аргументов командной строки и запуск внешнего процесса.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#include "core/FirewallManager.hpp"
#include "common/AppLogger.hpp"
#include <mutex>
#include <QHostAddress>
#include <QProcess>
#include <QStringList>

// Глобальный мьютекс для защиты множества заблокированных IP
static std::mutex g_firewallMutex;

/**
 * @brief Блокировка IP-адреса.
 * @param ip Строка с IP.
 */
void FirewallManager::blockIp(const std::string& ip) {
  // Проверка валидности IP-адреса
  QHostAddress addr(QString::fromStdString(ip));
  if (addr.isNull()) {
    AppLogger::get()->error("[SECURITY] Некорректный формат IP: '{}'. Операция отменена.", ip);
    return;
  }

  std::lock_guard<std::mutex> lock(g_firewallMutex);
  // Если IP уже заблокирован — ничего не делаем
  if (activeBlockedIps_.count(ip)) return;

#ifdef _WIN32
  AppLogger::get()->warn("Mitigation: Выполнение блокировки IP {}", ip);
  
  QString program = "netsh";
  QStringList arguments;
  // Формирование команды для добавления правила блокировки
  arguments << "advfirewall" << "firewall" << "add" << "rule"
        << "name=DDoS_Block_" + addr.toString()
        << "dir=in" << "action=block"
        << "remoteip=" + addr.toString();

  // Запуск процесса асинхронно
  QProcess::startDetached(program, arguments);
  
  activeBlockedIps_.insert(ip);
#else
  AppLogger::get()->warn("Mitigation: Блокировка IP {} не реализована для данной ОС", ip);
#endif
}

/**
 * @brief Разблокировка конкретного IP-адреса.
 * @param ip Строка с IP.
 */
void FirewallManager::unblockIp(const std::string& ip) {
  QHostAddress addr(QString::fromStdString(ip));
  if (addr.isNull()) {
    AppLogger::get()->error("[SECURITY] Некорректный IP для разблокировки: {}", ip);
    return;
  }

  std::lock_guard<std::mutex> lock(g_firewallMutex);
  if (!activeBlockedIps_.count(ip)) return;

#ifdef _WIN32
  AppLogger::get()->info("Mitigation: Удаление блокировки для IP {}", ip);

  QString program = "netsh";
  QStringList arguments;
  // Удаление правила по уникальному имени
  arguments << "advfirewall" << "firewall" << "delete" << "rule"
        << "name=DDoS_Block_" + addr.toString();

  QProcess::startDetached(program, arguments);
#endif
  activeBlockedIps_.erase(ip);
}

/**
 * @brief Снятие всех активных блокировок сессии.
 */
void FirewallManager::unblockAllIps() {
  std::lock_guard<std::mutex> lock(g_firewallMutex);
#ifdef _WIN32
  for (const auto& ip : activeBlockedIps_) {
    AppLogger::get()->info("Mitigation: Удаление блокировки для IP {}", ip);

    QString program = "netsh";
    QStringList arguments;
    arguments << "advfirewall" << "firewall" << "delete" << "rule"
        << "name=DDoS_Block_" + QString::fromStdString(ip);

    QProcess::startDetached(program, arguments);
  }
#endif
  activeBlockedIps_.clear();
}

/**
 * @brief Полная очистка правил (в том числе оставшихся после сбоев).
 */
void FirewallManager::clearAllRules() {
  std::lock_guard<std::mutex> lock(g_firewallMutex);
#ifdef _WIN32
  // Удаление всех правил, начинающихся на DDoS_Block_ через wildcard
  QString program = "netsh";
  QStringList arguments;
  arguments << "advfirewall" << "firewall" << "delete" << "rule" << "name=DDoS_Block_*";
  QProcess::startDetached(program, arguments);
#endif
  activeBlockedIps_.clear();
}

/**
 * @brief Получение текущего списка заблокированных адресов.
 * @return Вектор строк IP.
 */
std::vector<std::string> FirewallManager::getBlockedIps() const {
  std::lock_guard<std::mutex> lock(g_firewallMutex);
  return std::vector<std::string>(activeBlockedIps_.begin(), activeBlockedIps_.end());
}
