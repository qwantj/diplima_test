# Руководство разработчика

**Версия:** 2.2  
**Дата актуализации:** 21.05.2026

---

## 1. Требования к рабочей среде

### Обязательные инструменты

| Инструмент | Версия | Примечания |
|---|---|---|
| Visual Studio 2022 | 17.x | Desktop C++ workload |
| Qt 6 | 6.6+ | Компонент MSVC 2022 64-bit |
| vcpkg | актуальная | `C:\vcpkg` (рекомендуется) |
| Git | 2.x | — |
| CMake | 3.16+ | Входит в VS 2022 |
| ONNX Runtime | 1.18+ | Ручная установка в `C:\Lib\onnxruntime` |
| Npcap | 1.75+ | WinPcap-compatible mode |
| PostgreSQL | 14+ | Опционально, для хранения данных |

### Структура директорий ONNX Runtime

```
C:\Lib\onnxruntime\
├── include\
│   └── onnxruntime_cxx_api.h
│   └── ...
└── lib\
    ├── onnxruntime.lib
    └── onnxruntime.dll
```

---

## 2. Первоначальная настройка

### Шаг 1: Установка зависимостей vcpkg

```powershell
# Предполагается, что vcpkg установлен в C:\vcpkg
C:\vcpkg\vcpkg install pcapplusplus:x64-windows
C:\vcpkg\vcpkg install libpqxx:x64-windows
C:\vcpkg\vcpkg install postgresql:x64-windows
```

### Шаг 2: Конфигурация CMake (MSVC)

```powershell
# В Developer PowerShell for VS 2022
cmake -S C:\Dev\CXX\diploma_test -B C:\Dev\CXX\diploma_test\build_msvc `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.6.0/msvc2019_64"
```

### Шаг 3: Сборка

```powershell
cmake --build C:\Dev\CXX\diploma_test\build_msvc --config Release
# или Debug:
cmake --build C:\Dev\CXX\diploma_test\build_msvc --config Debug
```

### Шаг 4: Разворачивание Qt (windeployqt)

```powershell
cd C:\Dev\CXX\diploma_test\build_msvc\Release
C:\Qt\6.6.0\msvc2019_64\bin\windeployqt.exe ddos_monitor.exe
```

---

## 3. Соглашения о коде

### Стандарт

- **C++17** (строго, MSVC `/std:c++17`)
- Запрещены C-style casts, raw `new`/`delete` (использовать `std::make_unique`)
- Запрещены глобальные мьютексы в горячем цикле захвата

### Именование

| Элемент | Стиль | Пример |
|---|---|---|
| Классы, структуры | PascalCase | `TrafficMonitor`, `DetectionResult` |
| Методы, функции | camelCase | `startCapture()`, `enqueueEvent()` |
| Переменные | camelCase | `totalPackets`, `queueSize` |
| Приватные поля | camelCase + `_` | `socket_`, `dbManager_` |
| Константы | SCREAMING_SNAKE | `MAX_QUEUE_SIZE`, `FLUSH_INTERVAL_MS` |
| Файлы | PascalCase.cpp/.hpp | `DetectionEngine.cpp` |

### Управление памятью

```cpp
// ✅ Правильно
auto engine = std::make_unique<DetectionEngine>();
auto conn = std::make_shared<pqxx::connection>(connStr);

// ❌ Запрещено
DetectionEngine* engine = new DetectionEngine();
```

### Потокобезопасность

```cpp
// ✅ Lock-free для горячего цикла (moodycamel)
moodycamel::ConcurrentQueue<PacketBuffer*> queue;
queue.enqueue(pkt);      // В потоке захвата
queue.try_dequeue(pkt);  // В потоке инференса

// ✅ shared_mutex для редких операций (hot swap модели)
mutable std::shared_mutex mutex_;
std::shared_lock lock(mutex_);   // predict() — concurrent read
std::unique_lock lock(mutex_);   // hotSwapModel() — exclusive write

// ❌ Не использовать в горячем цикле
std::lock_guard<std::mutex> guard(mutex_); // Блокирует захват
```

---

## 4. Добавление новых признаков для ML

### Шаг 1: Python — обновить пайплайн

```python
# scripts/kaggle_pipeline.py
# Добавить новый признак в список features:
features = [
    "Flow Duration",
    "Total Fwd Packets",
    # ... существующие ...
    "NEW_FEATURE_NAME",  # ← добавить
]
```

### Шаг 2: Python — переобучить модель

```bash
python scripts/kaggle_pipeline.py
# Генерирует:
# models/rf_model.onnx
# models/rf_scaler_params.json
```

### Шаг 3: C++ — добавить расчёт в FeatureExtractor

Открыть [`src/network/FeatureExtractor.cpp`](../src/network/FeatureExtractor.cpp), в методе `computeFeaturesBatch()`:

```cpp
// 1. Добавить накопление метрики в processPacket():
void FeatureExtractor::processPacket(PacketBuffer* pkt) {
    // ... существующий код ...
    // Пример: считать SYN/ACK ratio
    flow.newMetric += synFlag ? 1 : 0;
}

// 2. Добавить расчёт в buildFeatureVector():
double newFeatureValue = (flow.newMetric > 0)
    ? (double)flow.synPackets / flow.newMetric
    : 0.0;
features.push_back(newFeatureValue);
```

> ⚠️ **Критично:** порядок признаков в C++ должен строго совпадать с порядком в `scaler_params.json`!

### Шаг 4: Обновить scaler JSON

```json
{
  "features": ["Flow Duration", ..., "NEW_FEATURE_NAME"],
  "mean":     [1190047.0, ..., 0.5],
  "scale":    [6487791.0, ..., 0.2],
  "use_log1p": false
}
```

---

## 5. Добавление новых виджетов в Dashboard

### Шаг 1: Создать виджет

```cpp
// src/monitor_ui/MyNewWidget.hpp
#pragma once
#include <QWidget>
#include "common/Protocol.hpp"

class MyNewWidget : public QWidget {
    Q_OBJECT
public:
    explicit MyNewWidget(QWidget* parent = nullptr);

public slots:
    void updateData(const DetectionResult& result);
};
```

### Шаг 2: Добавить в CMakeLists.txt

```cmake
# В цели ddos_monitor:
add_executable(ddos_monitor
    # ... существующие ...
    src/monitor_ui/MyNewWidget.cpp
)
```

### Шаг 3: Подключить к DataBridge

```cpp
// В MainWindow::setupConnections():
connect(dataBridge_, &DataBridge::realtimeStatsReceived,
        myNewWidget_, &MyNewWidget::updateData);
```

### Шаг 4: Добавить в DashboardWidget или стек страниц

```cpp
// В DashboardWidget::setupAnalytics() или в monitor_main.cpp:
stackedWidget_->addWidget(myNewWidget_);
```

---

## 6. Работа с базой данных

### Добавление новой таблицы

```cpp
// src/common/DatabaseManager.cpp → DatabaseManager::ensureTables()
tx.exec(R"(
    CREATE TABLE IF NOT EXISTS my_new_table (
        id         SERIAL PRIMARY KEY,
        session_id INT REFERENCES sessions(id),
        timestamp  TIMESTAMP DEFAULT NOW(),
        my_field   TEXT
    )
)");
```

### Добавление async записи

```cpp
// 1. Объявить очередь и структуру в DatabaseManager.hpp:
struct MyEntry { int sessionId; QString myField; QDateTime timestamp; };
moodycamel::ConcurrentQueue<MyEntry> myQueue_;

// 2. Метод enqueue:
void enqueueMyEvent(int sessionId, const QString& field) {
    myQueue_.enqueue({sessionId, field, QDateTime::currentDateTime()});
}

// 3. Метод flush (вызывать из flushEvents или создать отдельный):
void flushMyEvents(pqxx::connection& conn) {
    MyEntry entry;
    std::vector<MyEntry> batch;
    while (myQueue_.try_dequeue(entry)) {
        batch.push_back(entry);
        if ((int)batch.size() >= MAX_EVENTS_PER_FLUSH) break;
    }
    if (!batch.empty()) {
        pqxx::work tx(conn);
        auto stream = pqxx::stream_to::table(tx, {"my_new_table"},
            {"session_id", "timestamp", "my_field"});
        for (auto& e : batch) {
            stream << std::make_tuple(e.sessionId, toTimestampStr(e.timestamp),
                                      e.myField.toStdString());
        }
        stream.complete();
        tx.commit();
    }
}
```

---

## 7. Добавление ML-модели

### Требования к ONNX-модели

- Входной тензор: `float32`, shape `[1, N]` где N = количество признаков
- Выходных тензора: 2
  - `output_label`: `int64`, shape `[1]` — предсказанный класс (0 или 1)
  - `output_probability`: `float32`, shape `[1, 2]` — вероятности классов

### Конвертация модели из scikit-learn

```python
from sklearn.ensemble import RandomForestClassifier
from skl2onnx import convert_sklearn
from skl2onnx.common.data_types import FloatTensorType

model = RandomForestClassifier(n_estimators=100)
model.fit(X_train, y_train)

initial_type = [('float_input', FloatTensorType([None, X_train.shape[1]]))]
onnx_model = convert_sklearn(model, initial_types=initial_type,
                              target_opset=12)

with open("models/rf_model.onnx", "wb") as f:
    f.write(onnx_model.SerializeToString())
```

### Проверка модели

```powershell
# Запустить коллектор с новой моделью
.\ddos_collector.exe -i "WiFi" --model models/new_model.onnx
# Следить за логом ddos_collector.log
```

---

## 8. Отладка и профилирование

### Режим Debug

```powershell
cmake --build build_msvc --config Debug
.\build_msvc\Debug\ddos_collector.exe -i "WiFi" -m models/rf_model.onnx
```

### Просмотр логов

```powershell
# Collector лог
Get-Content ddos_collector.log -Tail 50 -Wait

# Monitor лог
Get-Content ddos_monitor.log -Tail 50 -Wait
```

### Проверка очереди

```cpp
// В inferenceLoop():
AppLogger::get()->debug("Queue size: {}, Dropped: {}",
    monitor_.queueSize(), monitor_.droppedPackets());
```

### Запуск unit-тестов

```powershell
cd build_msvc
ctest -C Release --output-on-failure
```

---

## 9. Контрольный список перед Pull Request

- [ ] Код компилируется без предупреждений (`/W4`)
- [ ] Unit тесты проходят (`ctest -C Release`)
- [ ] Нет сырых `new`/`delete` без парного умного указателя
- [ ] Потокобезопасность проверена (lock-free для горячего цикла)
- [ ] `config.json` не содержит реальных паролей
- [ ] Документация обновлена (если изменился интерфейс)
- [ ] Признаки ML (если изменены) синхронизированы с scaler JSON

---

## 10. Полезные ресурсы

| Ресурс | URL |
|---|---|
| PcapPlusPlus Docs | https://pcapplusplus.github.io/docs/ |
| ONNX Runtime C++ API | https://onnxruntime.ai/docs/api/c/ |
| libpqxx Docs | https://pqxx.org/libpqxx/html/ |
| Qt 6 Docs | https://doc.qt.io/qt-6/ |
| moodycamel Queue | https://github.com/cameron314/concurrentqueue |
| spdlog | https://github.com/gabime/spdlog |
| CICIDS2017 Dataset | https://www.unb.ca/cic/datasets/ids-2017.html |