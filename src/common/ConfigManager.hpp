/**
 * @file ConfigManager.hpp
 * @brief Модуль управления конфигурацией (заголовок).
 *
 * Назначение: Загрузка и хранение параметров приложения из JSON-файла.
 * Входные данные: Файл config.json.
 * Результаты: Структура AppConfig с параметрами системы.
 * Метод решения: Использование библиотеки nlohmann/json для десериализации.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#pragma once // Директива защиты от повторного включения

#include <string>
#include <nlohmann/json.hpp>

/**
 * @struct AppConfig
 * @brief Структура для хранения настроек приложения.
 */
struct AppConfig {
  // Общие настройки Коллектора и Монитора
  std::string collectorHost = "127.0.0.1";
  std::string tcpBindHost = "127.0.0.1";
  int tcpPort = 50050;

  // Параметры подключения к базе данных
  std::string dbHost = "localhost";
  int dbPort = 5432;
  std::string dbName = "ddos_detection_db";
  std::string dbUser = "";
  std::string dbPass = "";

  // Параметры машинного обучения
  std::string defaultModel = "models/rf_model.onnx";
  std::string defaultEp = "cpu";
  double inferenceWindowSec = 1.0;

  // Сетевые настройки
  std::string defaultInterface = "";
  int maxQueueSize = 500000;
};

class ConfigManager {
public:
  // Загрузка конфигурации из файла
  static bool load(const std::string& path, AppConfig& config);
  
  // Сохранение текущей конфигурации
  static bool save(const std::string& path, const AppConfig& config);
};
