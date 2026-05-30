/**
 * @file ModelInferencer.cpp
 * @brief Модуль инференса ML-моделей (реализация).
 *
 * Назначение: Реализация методов выполнения классификации трафика через ONNX Runtime.
 * Входные данные: Нормализованный вектор признаков.
 * Результаты: Прогноз модели (метка класса и вероятность).
 * Метод решения: Загрузка графа вычислений ONNX, подготовка тензоров и запуск сессии.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#include "ml/ModelInferencer.hpp"
#include "common/AppLogger.hpp"
#include <filesystem>

// Конструктор: инициализация окружения ONNX Runtime
ModelInferencer::ModelInferencer() {
#ifdef ONNX_RUNTIME_AVAILABLE
  env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "DDoSDetector");
#endif
}

// Деструктор
ModelInferencer::~ModelInferencer() { unloadModel(); }

// Получение имени текущей модели (с блокировкой на чтение)
std::string ModelInferencer::modelName() const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  return modelName_;
}

// Получение пути к файлу текущей модели
std::string ModelInferencer::modelPath() const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  return modelPath_;
}

/**
 * @brief Загрузка модели и создание сессии инференса.
 * @param modelPath Путь к .onnx файлу.
 * @param ep Провайдер выполнения (cpu/dml).
 * @return true при успешной загрузке.
 */
bool ModelInferencer::loadModel(const std::string& modelPath, const std::string& ep) {
#ifdef ONNX_RUNTIME_AVAILABLE
  std::unique_lock<std::shared_mutex> lock(mutex_); // Эксклюзивная блокировка для записи
  try {
    sessionOptions_ = std::make_unique<Ort::SessionOptions>();
    sessionOptions_->SetIntraOpNumThreads(1); // Оптимизация для CPU
    sessionOptions_->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

    // Преобразование пути в wstring для поддержки Windows API
    std::wstring wpath(modelPath.begin(), modelPath.end());
    session_ = std::make_unique<Ort::Session>(*env_, wpath.c_str(), *sessionOptions_);

    // Получение имен входных и выходных тензоров
    Ort::AllocatorWithDefaultOptions allocator;
    inputNames_.clear();
    outputNames_.clear();
    inputNamePtrs_.clear();
    outputNamePtrs_.clear();

    size_t numInputs = session_->GetInputCount();
    inputNames_.reserve(numInputs);
    inputNamePtrs_.reserve(numInputs);
    for (size_t i = 0; i < numInputs; i++) {
      auto name = session_->GetInputNameAllocated(i, allocator);
      inputNames_.push_back(name.get());
    }

    size_t numOutputs = session_->GetOutputCount();
    outputNames_.reserve(numOutputs);
    outputNamePtrs_.reserve(numOutputs);
    for (size_t i = 0; i < numOutputs; i++) {
      auto name = session_->GetOutputNameAllocated(i, allocator);
      outputNames_.push_back(name.get());
    }

    // Сохранение указателей на имена для использования в Run()
    for (auto& n : inputNames_) inputNamePtrs_.push_back(n.c_str());
    for (auto& n : outputNames_) outputNamePtrs_.push_back(n.c_str());

    modelPath_ = modelPath;
    modelName_ = std::filesystem::path(modelPath).filename().string();

    AppLogger::get()->info("ModelInferencer: загружена модель '{}' (Входов: {}, Выходов: {}, EP={})",
      modelName_, numInputs, numOutputs, ep);
    return true;
  } catch (const Ort::Exception& e) {
    AppLogger::get()->error("ModelInferencer: ошибка загрузки ONNX: {}", e.what());
    session_.reset();
    return false;
  }
#else
  AppLogger::get()->error("ModelInferencer: ONNX Runtime не доступен в сборке");
  return false;
#endif
}

// Выгрузка модели и очистка ресурсов
void ModelInferencer::unloadModel() {
#ifdef ONNX_RUNTIME_AVAILABLE
  std::unique_lock<std::shared_mutex> lock(mutex_);
  session_.reset();
  sessionOptions_.reset();
  inputNames_.clear();
  outputNames_.clear();
  inputNamePtrs_.clear();
  outputNamePtrs_.clear();
#endif
}

// Проверка наличия загруженной сессии
bool ModelInferencer::isLoaded() const {
#ifdef ONNX_RUNTIME_AVAILABLE
  std::shared_lock<std::shared_mutex> lock(mutex_);
  return session_ != nullptr;
#else
  return false;
#endif
}

/**
 * @brief Выполнение предсказания.
 * @param features Нормализованный вектор признаков.
 * @return Пара (метка класса 0/1, уверенность классификатора).
 */
std::pair<int, float> ModelInferencer::predict(const std::vector<float>& features) {
#ifdef ONNX_RUNTIME_AVAILABLE
  std::shared_lock<std::shared_mutex> lock(mutex_); // Разделяемая блокировка для чтения
  if (!session_) return {0, 0.0f};

  try {
    // Подготовка входного тензора из вектора признаков
    auto memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<int64_t> inputShape = {1, (int64_t)features.size()};
    auto inputTensor = Ort::Value::CreateTensor<float>(
      memInfo, const_cast<float*>(features.data()),
      features.size(), inputShape.data(), inputShape.size());

    // 1. Получение метки класса (первый выходной тензор)
    const char* labelOutputName = outputNamePtrs_[0];
    auto outputs = session_->Run(
      Ort::RunOptions{nullptr},
      inputNamePtrs_.data(), &inputTensor, 1,
      &labelOutputName, 1);

    int label = 0;
    float confidence = 0.0f;

    if (!outputs.empty()) {
      auto& labelTensor = outputs[0];
      if (labelTensor.IsTensor()) {
        auto typeInfo = labelTensor.GetTensorTypeAndShapeInfo();
        auto elemType = typeInfo.GetElementType();
        if (elemType == ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64) {
          label = (int)*labelTensor.GetTensorData<int64_t>();
        } else if (elemType == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
          float val = *labelTensor.GetTensorData<float>();
          label = (int)(val > 0.5f);
          confidence = val;
        }
      }
    }

    // 2. Попытка получить вероятность из второго выходного тензора (если доступен)
    if (outputNamePtrs_.size() > 1) {
      try {
        const char* probOutputName = outputNamePtrs_[1];
        auto probOutputs = session_->Run(
          Ort::RunOptions{nullptr},
          inputNamePtrs_.data(), &inputTensor, 1,
          &probOutputName, 1);

        if (!probOutputs.empty() && probOutputs[0].IsTensor()) {
          auto& pt = probOutputs[0];
          auto typeInfo = pt.GetTensorTypeAndShapeInfo();
          auto elemType = typeInfo.GetElementType();
          if (elemType == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
            auto shape = typeInfo.GetShape();
            size_t count = 1;
            for (auto s : shape) if (s > 0) count *= s;
            const float* probs = pt.GetTensorData<float>();
            // Выбор вероятности положительного класса (индекс 1)
            confidence = (count >= 2) ? probs[1] : probs[0];
          }
        } else {
          // Если выход не тензор (например, Map в sklearn), устанавливаем константу
          confidence = (label == 1) ? 0.99f : 0.01f;
        }
      } catch (...) {
        confidence = (label == 1) ? 0.99f : 0.01f;
      }
    }

    return {label, confidence};
  } catch (const Ort::Exception& e) {
    AppLogger::get()->error("ModelInferencer: ошибка инференса: {}", e.what());
    return {0, 0.0f};
  }
#else
  return {0, 0.0f};
#endif
}

/**
 * @brief Потокобезопасная горячая замена модели.
 * @param newModelPath Путь к новой модели.
 * @param ep Провайдер выполнения.
 * @return true при успехе.
 */
bool ModelInferencer::hotSwapModel(const std::string& newModelPath, const std::string& ep) {
  AppLogger::get()->info("ModelInferencer: горячая замена модели на '{}'", newModelPath);
  unloadModel();
  return loadModel(newModelPath, ep);
}
