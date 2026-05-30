/**
 * @file ModelInferencer.hpp
 * @brief Модуль инференса ML-моделей (заголовок).
 *
 * Назначение: Класс для выполнения предиктивного анализа с использованием ONNX Runtime.
 * Входные данные: Нормализованный вектор признаков сетевого трафика.
 * Результаты: Метка класса (атака/норма) и уровень уверенности классификатора.
 * Метод решения: Использование кроссплатформенного движка ONNX Runtime C++ API.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#pragma once // Директива защиты от повторного включения

#include <string>
#include <vector>
#include <memory>
#include <shared_mutex>

#ifdef ONNX_RUNTIME_AVAILABLE
#include <onnxruntime_cxx_api.h>
#endif

/**
 * @class ModelInferencer
 * @brief Класс-обертка над ONNX Runtime для выполнения инференса.
 */
class ModelInferencer {
public:
  ModelInferencer();
  ~ModelInferencer();

  // Загрузка модели из файла формата .onnx
  bool loadModel(const std::string& modelPath, const std::string& ep = "cpu");
  
  // Выгрузка текущей модели и освобождение ресурсов
  void unloadModel();
  
  // Проверка статуса готовности модели
  bool isLoaded() const;

  // Выполнение классификации. Возвращает {метка класса, уверенность}
  std::pair<int, float> predict(const std::vector<float>& features);

  // Геттеры метаданных
  std::string modelName() const;
  std::string modelPath() const;

  // Потокобезопасная горячая замена модели без остановки системы
  bool hotSwapModel(const std::string& newModelPath, const std::string& ep = "cpu");

private:
  std::string modelPath_; // Полный путь к файлу модели
  std::string modelName_; // Короткое имя модели для логов
  mutable std::shared_mutex mutex_; // Мьютекс для защиты при замене модели

#ifdef ONNX_RUNTIME_AVAILABLE
  std::unique_ptr<Ort::Env> env_;           // Окружение ORT
  std::unique_ptr<Ort::Session> session_;   // Текущая сессия инференса
  std::unique_ptr<Ort::SessionOptions> sessionOptions_; // Опции сессии

  std::vector<std::string> inputNames_;     // Имена входных тензоров
  std::vector<std::string> outputNames_;    // Имена выходных тензоров
  std::vector<const char*> inputNamePtrs_;  // Указатели имен (для API)
  std::vector<const char*> outputNamePtrs_; // Указатели имен (для API)
#endif
};
