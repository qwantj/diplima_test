#include "common/ConfigManager.hpp"
#include "common/AppLogger.hpp"
#include <fstream>
#include <iomanip>

bool ConfigManager::load(const std::string& path, AppConfig& config) {
    std::ifstream f(path);
    if (!f.is_open()) {
        AppLogger::get()->warn("ConfigManager: could not open '{}', using defaults.", path);
        return false;
    }

    try {
        nlohmann::json j;
        f >> j;

        config.collectorHost = j.value("collector_host", config.collectorHost);
        config.tcpBindHost = j.value("tcp_bind_host", config.tcpBindHost);
        config.tcpPort = j.value("tcp_port", config.tcpPort);

        if (j.contains("database")) {
            auto& db = j["database"];
            config.dbHost = db.value("host", config.dbHost);
            config.dbPort = db.value("port", config.dbPort);
            config.dbName = db.value("name", config.dbName);
            config.dbUser = db.value("user", config.dbUser);
            config.dbPass = db.value("password", config.dbPass);
        }

        if (j.contains("ml")) {
            auto& ml = j["ml"];
            config.defaultModel = ml.value("default_model", config.defaultModel);
            config.defaultEp = ml.value("default_ep", config.defaultEp);
            config.inferenceWindowSec = ml.value("window_sec", config.inferenceWindowSec);
        }

        if (j.contains("network")) {
            auto& net = j["network"];
            config.defaultInterface = net.value("default_interface", config.defaultInterface);
            config.maxQueueSize = net.value("max_queue_size", config.maxQueueSize);
        }

        AppLogger::get()->info("ConfigManager: loaded config from '{}'", path);
        return true;
    } catch (const std::exception& e) {
        AppLogger::get()->error("ConfigManager: error parsing '{}': {}", path, e.what());
        return false;
    }
}

bool ConfigManager::save(const std::string& path, const AppConfig& config) {
    nlohmann::json j;
    j["collector_host"] = config.collectorHost;
    j["tcp_bind_host"] = config.tcpBindHost;
    j["tcp_port"] = config.tcpPort;

    j["database"] = {
        {"host", config.dbHost},
        {"port", config.dbPort},
        {"name", config.dbName},
        {"user", config.dbUser},
        {"password", config.dbPass}
    };

    j["ml"] = {
        {"default_model", config.defaultModel},
        {"default_ep", config.defaultEp},
        {"window_sec", config.inferenceWindowSec}
    };

    j["network"] = {
        {"default_interface", config.defaultInterface},
        {"max_queue_size", config.maxQueueSize}
    };

    std::ofstream f(path);
    if (!f.is_open()) return false;

    f << std::setw(4) << j << std::endl;
    return true;
}
