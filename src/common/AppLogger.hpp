/**
 * @file AppLogger.hpp
 * @brief Модуль логирования (заголовок).
 *
 * Назначение: Обеспечение централизованного логирования событий в файл и консоль.
 * Входные данные: Текстовые сообщения, параметры форматирования.
 * Результаты: Записи в ddos_collector.log или ddos_monitor.log.
 * Метод решения: Использование библиотеки spdlog с поддержкой асинхронности и цветовой индикации.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#pragma once // Директива защиты от повторного включения

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>

/**
 * @class AppLogger
 * @brief Статический класс для управления глобальным объектом логгера.
 */
class AppLogger {
public:
  // Инициализация системы логирования
  static void init(const std::string& logFile = "ddos_monitor.log");
  
  // Получение указателя на экземпляр логгера
  static std::shared_ptr<spdlog::logger>& get();
  
  // Сброс настроек
  static void reset();

private:
  static std::shared_ptr<spdlog::logger> logger_;
};
