#pragma once

#include <string>
#include <set>
#include <vector>

class FirewallManager {
public:
    static FirewallManager& getInstance() {
        static FirewallManager instance;
        return instance;
    }

    void blockIp(const std::string& ip);
    void unblockIp(const std::string& ip);
    void unblockAllIps();
    void clearAllRules(); // Also cleans up any leftover rules from previous crashes

    std::vector<std::string> getBlockedIps() const;

private:
    FirewallManager() = default;
    ~FirewallManager() { clearAllRules(); }

    std::set<std::string> activeBlockedIps_;
};