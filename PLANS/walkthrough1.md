# Раздельные скейлеры для MLP и RF — Walkthrough

## Что сделано

### Python (обучение)

#### [train.py](file:///c:/Dev/CXX/diploma_test/scripts/train.py)
- Каждая модель обучается со **своим** `StandardScaler`
- Сохраняются файлы: `rf_scaler_params.json`, `mlp_scaler_params.json`, и `scaler_params.json` (обратная совместимость)

render_diffs(file:///c:/Dev/CXX/diploma_test/scripts/train.py)

#### [evaluate.py](file:///c:/Dev/CXX/diploma_test/scripts/evaluate.py)
- Добавлена функция `get_scaler_path_for_model()` — определяет файл скейлера по имени модели
- Нормализация теперь применяется внутри `evaluate_model()` с правильным скейлером одновременно

render_diffs(file:///c:/Dev/CXX/diploma_test/scripts/evaluate.py)

---

### C++ (инференс)

#### [DetectionEngine.hpp](file:///c:/Dev/CXX/diploma_test/src/core/DetectionEngine.hpp) / [DetectionEngine.cpp](file:///c:/Dev/CXX/diploma_test/src/core/DetectionEngine.cpp)
- `switchModel()` теперь принимает `scalerPath` и перезагружает скейлер

#### [main.cpp](file:///c:/Dev/CXX/diploma_test/src/main.cpp)
- Добавлена функция `getScalerPath()`: `rf_model.onnx → rf_scaler_params.json`
- `setupEngine()` и `onModelChanged()` автоматически подбирают скейлер
- Fallback на `scaler_params.json` если специфический файл не найден

render_diffs(file:///c:/Dev/CXX/diploma_test/src/main.cpp)

---

## Следующие шаги
1. Переобучить модели: `cd scripts && python train.py`
2. Скопировать `rf_scaler_params.json` и `mlp_scaler_params.json` в build-директорию (рядом с ONNX моделями)
3. Собрать и запустить C++ приложение
