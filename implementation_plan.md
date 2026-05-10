# Восстановление кодовой базы DiplomDDoS

## Описание

Пересоздание всех утраченных исходных файлов проекта `DiplomDDoS` (ddos_collector, ddos_monitor, ddos_core) на основе:
- MOC-файлов (полные Qt-сигнатуры классов)
- `.tlog` / `.d` файлов (точные include-зависимости)
- `.vcxproj.filters` (структура файлов)
- Логов работы приложений
- CMakeCache (зависимости: vcpkg, spdlog, nlohmann_json, PcapPlusPlus, PostgreSQL, ONNX Runtime, Qt 6.10.2)
- Прошлых разговоров Gemini (conversation logs `59fc7621`, `5be9948b`, `30c8ff39`)

## Архитектура (реконструкция)

```
DiplomDDoS
├── ddos_core (static lib) — общие модули
├── ddos_collector (exe) — захват трафика, ONNX-инференс, TCP-сервер, PostgreSQL  
└── ddos_monitor (exe) — Qt GUI клиент, TCP-клиент, визуализация
```

## Предлагаемые изменения

### Фаза 1: CMakeLists.txt и инфраструктура

#### [MODIFY] [CMakeLists.txt](file:///c:/Dev/CXX/diploma_test/CMakeLists.txt)
Полная замена — текущий является устаревшим scaffold. Новый CMake:
- Проект `DiplomDDoS`, C++17, MSVC
- FetchContent: spdlog, nlohmann_json
- find_package: Qt6 (Core Network Sql Widgets Charts OpenGL OpenGLWidgets), PcapPlusPlus, PostgreSQL
- Цели: `ddos_core` (STATIC), `ddos_collector` (EXE), `ddos_monitor` (EXE)
- Post-build: копирование onnxruntime.dll, libpq.dll, windeployqt

---

### Фаза 2: ddos_core (14 файлов)

#### [NEW] `src/concurrentqueue.h`
Cameron Desrochers' lock-free concurrent queue (header-only, MIT)

#### [NEW] `src/common/AppLogger.hpp` / `.cpp`
- Враппер над spdlog
- Статические методы init()/get()
- Инициализация с файлом `ddos_monitor.log` или `ddos_collector.log`

#### [NEW] `src/common/Protocol.hpp` / `.cpp`
Из MOC-данных TcpClient/DataBridge:
- `struct DetectionResult` — основная структура данных:
  - label (int), probability (float), timestamp, features (JSON array)
  - pps, total_packets, tcp/udp/icmp/syn/fin/rst counts
- `struct SessionInfo` — id, start_time, end_time, total_events
- Сериализация/десериализация через nlohmann::json
- Типы сообщений: `snapshot`, `stats`, `buffer`, `notification`, `config`

#### [NEW] `src/common/DatabaseManager.hpp` / `.cpp`
Из MOC (DataBridge) + логов:
- Наследует QObject
- Подключение к PostgreSQL через Qt SQL (драйвер QPSQL)
- Асинхронная запись (flush thread каждые 5000ms)
- Таблицы: detection_events, sessions, stats_snapshots
- Методы: connect(), disconnect(), insertEvent(), getSessions(), getEventsForSession()

#### [NEW] `src/common/FileBuffer.hpp` / `.cpp`
- Буферизация данных в файл
- Для промежуточного хранения, если БД недоступна

#### [NEW] `src/common/SystemMetricsCollector.hpp` / `.cpp`
- Сбор CPU/RAM метрик
- Windows API (GetSystemTimes, GlobalMemoryStatusEx)

#### [NEW] `src/common/TcpServer.hpp` / `.cpp`
Из MOC:
- Наследует `QTcpServer`
- Private slots: `onClientDisconnected()`, `onReadyRead()`
- Порт 50050 (из логов)
- Буферизация последних N строк для новых клиентов
- JSON-протокол + length-prefixed framing

#### [NEW] `src/common/TcpClient.hpp` / `.cpp`
Из MOC:
- Наследует `QObject`, содержит `QTcpSocket*`
- Signals: `connectedToServer()`, `disconnectedFromServer()`, 
  `statsReceived(DetectionResult, uint64_t)`, `snapshotReceived(float pps, uint64_t totalPackets, int label)`,
  `bufferReceived(vector<QByteArray>)`, `notificationReceived(QString, QJsonObject)`
- Private slots: `onConnected()`, `onDisconnected()`, `onReadyRead()`, `tryReconnect()`
- Auto-reconnect с таймером

#### [NEW] `src/common/DataBridge.hpp` / `.cpp`
Из MOC:
- Наследует `QObject`
- Связывает TcpClient + DatabaseManager
- Signals: `collectorConnectionChanged(bool)`, `databaseConnectionChanged(bool)`,
  `realtimeStatsReceived(DetectionResult, uint64_t)`, `realtimeSnapshotReceived(float, uint64_t, int)`,
  `historicalBufferReceived(vector<QByteArray>)`, `historicalStatsReceived(vector<DetectionResult>)`,
  `notificationReceived(QString, QJsonObject)`
- Public slot: `sendBpfConfig(bool)`
- Private slots: `onCollectorConnected()`, `onCollectorDisconnected()`, `onHistoricalBufferReceived(vector<QByteArray>)`

#### [NEW] `src/network/TrafficMonitor.hpp` / `.cpp`
- PcapPlusPlus PcapLiveDevice
- Буфер 64 MB
- Callback-based захват пакетов
- Подсчёт статистик: total, tcp, udp, icmp, syn, fin, rst
- BPF фильтрация
- InferenceWindow (2 секунды, из логов)

#### [NEW] `src/network/FeatureExtractor.hpp` / `.cpp`
- Извлечение 8 фичей (из существующего кода + scaler_params.json)
- Нормализация (StandardScaler)

#### [NEW] `src/network/PcapDumper.hpp` / `.cpp`
- Запись пакетов в .pcap при обнаружении атаки
- Директория pcap_dumps/session_YYYYMMDD_HHMMSS/

#### [NEW] `src/ml/ModelInferencer.hpp` / `.cpp`
- ONNX Runtime C++ API
- Загрузка модели, создание сессии
- Inference с динамическими input/output nodes
- CPU ExecutionProvider
- Из логов: output_label, output_probability

#### [NEW] `src/core/DetectionEngine.hpp` / `.cpp`
- Координирует TrafficMonitor + FeatureExtractor + ModelInferencer
- InferenceWindow (2 сек)
- Генерирует DetectionResult

---

### Фаза 3: ddos_collector (1 файл)

#### [NEW] `src/collector_main.cpp`
- QCoreApplication
- Инициализация: AppLogger, DetectionEngine, DatabaseManager, TcpServer, SystemMetricsCollector
- Обработка сигнала завершения (SIGINT/SIGTERM)
- Основной цикл: capture → inference → broadcast → store

---

### Фаза 4: ddos_monitor (7 файлов)

#### [NEW] `src/monitor_ui/ThemePalette.hpp` / `.cpp`
- enum ThemeMode { Light, Dark }
- Цветовые палитры для графиков и виджетов

#### [NEW] `src/monitor_ui/DashboardWidget.hpp` / `.cpp`
Из MOC — очень развитый виджет:
- Наследует QWidget
- Классы внутри: HeatmapWidget (QWidget), AlertGridWidget (QWidget), InteractiveChartView (QChartView)
- Signal: `bpfToggled(bool)`
- Public slots: `updateRealtime(DetectionResult, uint64_t)`, `updateSnapshot(float, uint64_t, int)`,
  `updateConnectionStatus(bool)`, `loadHistory(vector<DetectionResult>)`, `applyTheme(ThemeMode)`, 
  `resetZoom()`, `getTabAnalytics() → QWidget*`, `getTabSystem() → QWidget*`
- Private slot: `onUserInteracted()`
- Графики: QLineSeries (PPS), QAreaSeries, QDateTimeAxis, QValueAxis
- Qt Charts: QChart, QChartView, QPieSeries, QBarSeries, QStackedBarSeries, QBarCategoryAxis

#### [NEW] `src/monitor_ui/LogWidget.hpp` / `.cpp`
Из MOC:
- Наследует QWidget
- Public slots: `addEvent(DetectionResult)`, `loadHistory(vector<DetectionResult>)`, `clear()`
- Private slots: `onFilterChanged(int)`, `onBatchUpdate()`
- QTableWidget для отображения событий

#### [NEW] `src/monitor_ui/SessionWidget.hpp` / `.cpp`
Из MOC:
- Наследует QWidget
- Signal: `sessionSelected(int sessionId)`
- Public slot: `loadSessions(vector<SessionInfo>)`
- Private slot: `onItemDoubleClicked(QTreeWidgetItem*, int)`
- QTreeWidget для отображения сессий

#### [NEW] `src/monitor_ui/EventHistoryWidget.hpp` / `.cpp`
Из MOC:
- Наследует QWidget
- Содержит TimelineWidget (QWidget — кастомная отрисовка)
- Public slots: `refreshData()`, `exportToCsv()`
- Private slot: `applyFilter()`
- Фильтрация + экспорт в CSV

#### [NEW] `src/monitor_main.cpp`
Из MOC:
- `class MainWindow : public QMainWindow` (определён прямо в .cpp, поэтому monitor_main.moc)
- Private slots: `onCollectorStatusChanged(bool)`, `onDatabaseStatusChanged(bool)`,
  `onSessionSelected(int)`, `pollSessions()`
- Sidebar с вкладками (из conversation log): Dashboard, Deep Analytics, Event Log, Security Incidents, Sessions History
- QStackedWidget + QListWidget для навигации
- QSystemTrayIcon
- QStatusBar с индикаторами подключения
- Зависимости (из .d): QSettings, QPropertyAnimation, QTimer, QThread

---

### Фаза 5: Удаление устаревших файлов

#### [DELETE] `src/main.cpp` — устаревший scaffold
#### [DELETE] `src/TrafficMonitor.h` / `.cpp` — заменены на src/network/
#### [DELETE] `src/FeatureExtractor.h` / `.cpp` — заменены на src/network/

---

## План верификации

### Сборка
```powershell
cmake -S . -B build_restore -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:/Dev/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DCMAKE_PREFIX_PATH=C:/qt/6.10.2/msvc2022_64
cmake --build build_restore --config Release
```

### Тестирование
1. `ddos_collector.exe` запускается, загружает ONNX-модель, слушает порт 50050
2. `ddos_monitor.exe` запускается, подключается к collector, отображает GUI
3. Проверка логов: формат совпадает с `collector_debug.txt` / `ddos_monitor.log`

## Открытые вопросы

> [!IMPORTANT]
> 1. **ONNX-модель**: файлы `models/rf_model.onnx` и `models/scaler_params.json` существуют? Проверьте в models/
> 2. **PostgreSQL**: нужна ли БД для работы, или collector может работать автономно?
> 3. **Приоритет**: начать с ddos_core + collector (бэкенд) или с monitor (GUI)?
