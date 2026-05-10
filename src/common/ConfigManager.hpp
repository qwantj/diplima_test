#pragma once

#include <string>
#include <nlohmann/json.hpp>

struct AppConfig {
    // Collector & Monitor shared
    std::string collectorHost = "localhost";
    int tcpPort = 50050;

    // Database
    std::string dbHost = "localhost";
    int dbPort = 5432;
    std::string dbName = "ddos_detection_db";
    std::string dbUser = "";
    std::string dbPass = "";

    // ML
    std::string defaultModel = "models/rf_model.onnx";
    std::string defaultEp = "cpu";
    double inferenceWindowSec = 2.0;

    // Network
    std::string defaultInterface = "";
    int maxQueueSize = 500000;
};

class ConfigManager {
public:
    static bool load(const std::string& path, AppConfig& config);
    static bool save(const std::string& path, const AppConfig& config);
};
