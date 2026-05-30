/**
 * @file FirewallManager.hpp
 * @brief Модуль управления межсетевым экраном (заголовок).
 *
 * Назначение: Класс для автоматического управления правилами блокировки IP-адресов в ОС Windows.
 * Входные данные: IP-адреса для блокировки/разблокировки.
 * Результаты: Изменение конфигурации Windows Firewall.
 * Метод решения: Использование системной утилиты netsh advfirewall через QProcess.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#pragma once // Директива защиты от повторного включения

#include <string>
#include <set>
#include <vector>

/**
 * @class FirewallManager
 * @brief Singleton-класс для взаимодействия с брандмауэром Windows.
 */
class FirewallManager {
public:
  // Получение единственного экземпляра класса
  static FirewallManager& getInstance() {
    static FirewallManager instance;
    return instance;
  }

  // Блокировка входящего трафика с указанного IP
  void blockIp(const std::string& ip);
  
  // Удаление правила блокировки для IP
  void unblockIp(const std::string& ip);
  
  // Снятие всех текущих блокировок
  void unblockAllIps();
  
  // Полная очистка всех правил с префиксом DDoS_Block_
  void clearAllRules();

  // Получение списка активно заблокированных IP
  std::vector<std::string> getBlockedIps() const;

private:
  FirewallManager() = default;
  ~FirewallManager() { clearAllRules(); }

  // Множество заблокированных IP-адресов (для идемпотентности)
  std::set<std::string> activeBlockedIps_;
};
