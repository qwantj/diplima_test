#include "common/AppLogger.hpp"

std::shared_ptr<spdlog::logger> AppLogger::logger_;

void AppLogger::init(const std::string& logFile) {
    if (logger_) return;

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto fileSink    = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile, true);

    logger_ = std::make_shared<spdlog::logger>("ddos",
        spdlog::sinks_init_list{consoleSink, fileSink});
    logger_->set_level(spdlog::level::info);
    logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    logger_->flush_on(spdlog::level::info);

    spdlog::set_default_logger(logger_);
}

std::shared_ptr<spdlog::logger>& AppLogger::get() {
    if (!logger_) init();
    return logger_;
}
