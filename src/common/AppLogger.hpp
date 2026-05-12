#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>

class AppLogger {
public:
    static void init(const std::string& logFile = "ddos_monitor.log");
    static std::shared_ptr<spdlog::logger>& get();
    static void reset();

private:
    static std::shared_ptr<spdlog::logger> logger_;
};
