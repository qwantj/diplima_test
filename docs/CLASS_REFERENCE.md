# Справочник по классам и компонентам

**Версия:** 2.2  
**Дата актуализации:** 21.05.2026

---

## Модуль `core/`

### `DetectionEngine`

**Файл:** [`src/core/DetectionEngine.hpp`](../src/core/DetectionEngine.hpp) / [`DetectionEngine.cpp`](../src/core/DetectionEngine.cpp)

Центральный оркестратор системы. Управляет жизненным циклом захвата трафика, извлечения признаков, ML-инференса и отслеживания инцидентов.

#### Публичный интерфейс

```cpp
class DetectionEngine {
public:
    // Типы коллбэков
    using ResultCallback   = std::function<void(const DetectionResult&)>;
    using IncidentCallback = std::function<void(const QDateTime& startTime,
                                                float duration,
                                                const std::string& attackerIp,
                                                float ppsMax,
                                                const std::string& typeLabel,
                                                float confidence)>;

    // Инициализация модели и скейлера
    bool init(const std::string& modelPath,
              const std::string& scalerPath = "",
              const std::string& ep = "cpu");

    // Запуск захвата с сетевого интерфейса
    bool startLive(const std::string& interfaceName);

    // Запуск воспроизведения PCAP-файла
    bool startReplay(const std::string& pcapPath);

    void stop();
    bool isRunning() const;

    // Установить BPF-фильтр для захвата
    void setBpfFilter(const std::string& filter);

    // Горячая замена модели (потокобезопасно)
    bool hotSwapModel(const std::string& modelPath, const std::string& ep = "cpu");

    // Запись PCAP-дампов
    void enablePcapDump(const std::string& dir);
    void disablePcapDump();

    // Управление активным противодействием
    void setMitigationEnabled(bool enabled);

    // Установить коллбэки
    void setResultCallback(ResultCallback cb);
    void setIncidentCallback(IncidentCallback cb);

    // Доступ к подсистемам
    TrafficMonitor&  trafficMonitor();
    FeatureExtractor& featureExtractor();
    ModelInferencer&  modelInferencer();

    static constexpr double INFERENCE_WINDOW_SEC = 1.0;
    static constexpr double NOISE_THRESHOLD_PPS  = 50.0;
};
```

#### Алгоритм работы `inferenceLoop()`

1. Сбрасываем `FeatureExtractor::reset()`.
2. В течение `INFERENCE_WINDOW_SEC` (1 с) читаем пакеты из очереди `TrafficMonitor`.
3. Каждый пакет обрабатывается `FeatureExtractor::processPacket()`.
4. По истечении окна вызываем `processWindow()`.
5. Если PPS < `NOISE_THRESHOLD_PPS` (50) → label=0, confidence=0 (нет шума).
6. Иначе: `computeNormalizedFeatures()` → `ModelInferencer::predict()` → `DetectionResult`.
7. Обновление состояния инцидента (`updateIncidentState()`).
8. Вызов `ResultCallback`.

---

### `FirewallManager`

**Файл:** [`src/core/FirewallManager.hpp`](../src/core/FirewallManager.hpp) / [`FirewallManager.cpp`](../src/core/FirewallManager.cpp)

Singleton для управления блокировкой IP-адресов через Windows Firewall (`netsh advfirewall`).

```cpp
class FirewallManager {
public:
    static FirewallManager& getInstance();

    void blockIp(const std::string& ip);
    void unblockIp(const std::string& ip);
    void unblockAllIps();
    void clearAllRules(); // Очистка правил при старте/завершении

    std::vector<std::string> getBlockedIps() const;
};
```

**Важно:** требует запуска от имени администратора.  
**Очистка:** при деструкторе `DetectionEngine` автоматически вызывается `clearAllRules()`.

---

## Модуль `network/`

### `TrafficMonitor`

**Файл:** [`src/network/TrafficMonitor.hpp`](../src/network/TrafficMonitor.hpp) / [`TrafficMonitor.cpp`](../src/network/TrafficMonitor.cpp)

Захват сетевых пакетов через PcapPlusPlus (Npcap). Предоставляет lock-free очередь буферов пакетов.

```cpp
class TrafficMonitor {
public:
    // Список всех доступных интерфейсов: {systemName, humanReadableName}
    static std::vector<std::pair<std::string, std::string>> listInterfaces();

    // Живой захват
    bool startCapture(const std::string& interfaceName);
    void stopCapture();
    bool isCapturing() const;

    // Воспроизведение PCAP
    bool replayPcap(const std::string& filePath);

    // BPF-фильтр (до старта захвата)
    void setBpfFilter(const std::string& filter);

    // Чтение пакетов из очереди (для inference потока)
    bool dequeuePacket(PacketBuffer*& packet);
    bool dequeuePacketTimed(PacketBuffer*& packet, std::chrono::microseconds timeout);
    void recycleBuffer(PacketBuffer* packet); // Вернуть в пул

    size_t   queueSize() const;
    uint64_t totalCaptured() const;
    uint64_t droppedPackets() const;

    static constexpr size_t MAX_QUEUE_SIZE = 500000;
};
```

**Пул буферов:** для предотвращения аллокаций в горячем цикле захвата используется `moodycamel::ConcurrentQueue<PacketBuffer*>`. Обработанные буферы возвращаются в пул через `recycleBuffer()`.

---

### `FeatureExtractor`

**Файл:** [`src/network/FeatureExtractor.hpp`](../src/network/FeatureExtractor.hpp) / [`FeatureExtractor.cpp`](../src/network/FeatureExtractor.cpp)

Вычисляет признаки для ML-модели из пакетов одного inference-окна.

```cpp
class FeatureExtractor {
public:
    // Загрузить параметры нормализации из JSON
    bool loadScalerParams(const std::string& jsonPath);

    // Обработать пакет в текущем окне
    void processPacket(PacketBuffer* pktBuf);

    // Вычислить нормализованные признаки (для наиболее нагруженного потока)
    std::vector<double> computeNormalizedFeatures();

    // Пакетный расчёт по всем потокам
    std::vector<std::vector<double>> computeNormalizedFeaturesBatch();

    // Заполнить поля телеметрии в DetectionResult
    void fillTelemetry(DetectionResult& result) const;

    void reset(); // Сброс состояния для нового окна

    // Установить скейлер по имени модели (auto-detect)
    void setModelScaler(const std::string& modelName);

    int    featureCount() const;
    size_t activeFlowsCount() const;
};
```

**8 извлекаемых признаков** (определяются `rf_scaler_params.json` / `mlp_scaler_params.json`):

| № | Название | Описание |
|---|---|---|
| 1 | Flow Duration | Длительность потока (мкс) |
| 2 | Total Fwd Packets | Число прямых пакетов потока |
| 3 | Total Backward Packets | Число обратных пакетов |
| 4 | Total Length of Fwd Packets | Суммарный размер прямых пакетов (байт) |
| 5 | Total Length of Bwd Packets | Суммарный размер обратных пакетов |
| 6 | Fwd Packet Length Mean | Средний размер прямого пакета |
| 7 | Bwd Packet Length Mean | Средний размер обратного пакета |
| 8 | Flow Packets/s | Пакетов в секунду для потока |

**Нормализация:** `X_norm = (X - mean) / scale` (StandardScaler). Если `use_log1p = true`: `X = log(1 + max(0, X))` перед нормализацией.

**Дополнительная телеметрия** (через `fillTelemetry`): распределение протоколов, гистограмма размеров пакетов, топ source IP, топ destination ports, топ targets, число уникальных источников.

---

### `PcapDumper`

**Файл:** [`src/network/PcapDumper.hpp`](../src/network/PcapDumper.hpp) / [`PcapDumper.cpp`](../src/network/PcapDumper.cpp)

Запись пакетов в файлы PCAP для последующего анализа.

```cpp
class PcapDumper {
public:
    bool startSession(const std::string& dir, const std::string& sessionName);
    void addPacket(const pcpp::RawPacket& packet);
    void commitWindow(int label); // Финализировать окно (метка: 0=norm, 1=attack)
    void close();
};
```

---

## Модуль `ml/`

### `ModelInferencer`

**Файл:** [`src/ml/ModelInferencer.hpp`](../src/ml/ModelInferencer.hpp) / [`ModelInferencer.cpp`](../src/ml/ModelInferencer.cpp)

Загрузка и выполнение ONNX-модели через ONNX Runtime.

```cpp
class ModelInferencer {
public:
    // Загрузить модель (ep: "cpu" | "cuda" | "dml")
    bool loadModel(const std::string& modelPath, const std::string& ep = "cpu");
    void unloadModel();
    bool isLoaded() const;

    // Предсказание: возвращает {label (0/1), confidence (0.0..1.0)}
    std::pair<int, float> predict(const std::vector<float>& features);

    std::string modelName() const;
    std::string modelPath() const;

    // Атомарная замена модели (thread-safe через shared_mutex)
    bool hotSwapModel(const std::string& newModelPath, const std::string& ep = "cpu");
};
```

**Поддерживаемые Execution Providers:**
- `cpu` — стандартный (всегда доступен)
- `cuda` — GPU через CUDA (требует CUDA toolkit)
- `dml` — DirectML для Windows GPU (входит в Windows 10/11)

**Поддерживаемые модели:**
- `rf_model.onnx` — Random Forest (752 KB)
- `mlp_model.onnx` — MLP нейросеть (47 KB)

---

## Модуль `common/`

### `Protocol` (namespace)

**Файл:** [`src/common/Protocol.hpp`](../src/common/Protocol.hpp) / [`Protocol.cpp`](../src/common/Protocol.cpp)

Сериализация/десериализация сообщений и структур данных.

```cpp
namespace Protocol {
    // Сериализация DetectionResult в JSON
    nlohmann::json serializeResult(const DetectionResult& r);

    // Десериализация DetectionResult из JSON
    DetectionResult deserializeResult(const nlohmann::json& j);

    // Сформировать stats-сообщение (QByteArray с length prefix)
    QByteArray serializeStats(const DetectionResult& r, uint64_t totalPackets);

    // Сформировать snapshot-сообщение
    QByteArray serializeSnapshot(float pps, uint64_t totalPackets, int currentLabel);

    // Сформировать cmd-сообщение
    QByteArray serializeCommand(const std::string& cmd, const nlohmann::json& data = {});

    // Сформировать notify-сообщение
    QByteArray serializeNotify(const std::string& event, const nlohmann::json& data = {});

    // Разобрать входящее сообщение → {type, data}
    std::pair<std::string, nlohmann::json> parseMessage(const QByteArray& raw);

    // Обрамить payload 4-байтовым заголовком длины
    QByteArray frame(const QByteArray& payload);
}
```

---

### `DatabaseManager`

**Файл:** [`src/common/DatabaseManager.hpp`](../src/common/DatabaseManager.hpp) / [`DatabaseManager.cpp`](../src/common/DatabaseManager.cpp)

Асинхронный менеджер PostgreSQL с двумя соединениями и lock-free очередями.

```cpp
class DatabaseManager : public QObject {
public:
    bool connectToDatabase(const QString& host, int port,
                           const QString& dbName,
                           const QString& user,
                           const QString& password);
    void disconnect();
    bool isConnected() const;

    // Управление сессиями
    int  createSession(const QString& iface, const QString& modelName);
    void closeSession(int sessionId, uint64_t totalPkts,
                      uint64_t attacks, uint64_t benign);
    std::vector<SessionInfo> getSessions(); // Последние 100

    // Постановка в очередь (non-blocking, thread-safe)
    void enqueueEvent(const DetectionResult& result);
    void enqueueSnapshot(float pps, uint64_t totalPackets,
                         int currentLabel, int sessionId);
    void enqueueSecurityEvent(int sessionId, const QDateTime& startTime,
                              float duration, const QString& attackerIp,
                              float ppsMax, const QString& typeLabel,
                              float confidence);

    // Получение истории
    std::vector<DetectionResult> getEventsForSession(int sessionId);
    std::vector<DetectionResult> getSecurityEvents(int limit = 100);
};
```

**Константы:**
- `FLUSH_INTERVAL_MS = 5000` — период флаша (мс)
- `MAX_EVENTS_PER_FLUSH = 1000` — макс. записей за один флаш

---

### `TcpServer`

**Файл:** [`src/common/TcpServer.hpp`](../src/common/TcpServer.hpp) / [`TcpServer.cpp`](../src/common/TcpServer.cpp)

Qt-based TCP-сервер коллектора.

```cpp
class TcpServer : public QObject {
Q_OBJECT
public:
    bool startListening(const QString& host, quint16 port);
    void broadcast(const QByteArray& data); // Отправить всем подключённым клиентам

signals:
    void commandReceived(const std::string& cmd, const nlohmann::json& data);
    void clientConnected();
    void clientDisconnected();
};
```

---

### `TcpClient`

**Файл:** [`src/common/TcpClient.hpp`](../src/common/TcpClient.hpp) / [`TcpClient.cpp`](../src/common/TcpClient.cpp)

Qt-based TCP-клиент монитора с автоматическим переподключением.

```cpp
class TcpClient : public QObject {
Q_OBJECT
public:
    void connectToHost(const QString& host, quint16 port);
    void sendCommand(const std::string& cmd, const nlohmann::json& data = {});
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void messageReceived(const std::string& type, const nlohmann::json& data);
    void rawBufferReceived(const std::vector<QByteArray>& messages);
};
```

---

### `DataBridge`

**Файл:** [`src/common/DataBridge.hpp`](../src/common/DataBridge.hpp) / [`DataBridge.cpp`](../src/common/DataBridge.cpp)

Агрегатор данных для монитора. Комбинирует `TcpClient` и `DatabaseManager`.

```cpp
class DataBridge : public QObject {
Q_OBJECT
public:
    void connectToCollector(const QString& host = "localhost", quint16 port = 50050);
    void connectToDatabase(const QString& host, int port,
                           const QString& dbName,
                           const QString& user, const QString& password);

    TcpClient*       tcpClient() const;
    DatabaseManager* databaseManager() const;

signals:
    void collectorConnectionChanged(bool connected);
    void databaseConnectionChanged(bool connected);
    void realtimeStatsReceived(const DetectionResult& result, uint64_t totalPackets);
    void realtimeSnapshotReceived(float pps, uint64_t totalPackets, int currentLabel);
    void historicalStatsReceived(const std::vector<DetectionResult>& events);
    void notificationReceived(const QString& event, const QJsonObject& data);

public slots:
    void sendBpfConfig(bool enable);
    void sendDumpConfig(bool enable);
};
```

---

### `ConfigManager`

**Файл:** [`src/common/ConfigManager.hpp`](../src/common/ConfigManager.hpp) / [`ConfigManager.cpp`](../src/common/ConfigManager.cpp)

```cpp
struct AppConfig {
    std::string collectorHost = "127.0.0.1";
    std::string tcpBindHost   = "127.0.0.1";
    int         tcpPort       = 50050;

    std::string dbHost   = "localhost";
    int         dbPort   = 5432;
    std::string dbName   = "ddos_detection_db";
    std::string dbUser   = "";
    std::string dbPass   = "";

    std::string defaultModel = "models/rf_model.onnx";
    std::string defaultEp    = "cpu";
    double      inferenceWindowSec = 1.0;

    std::string defaultInterface = "";
    int         maxQueueSize     = 500000;
};

class ConfigManager {
public:
    static bool load(const std::string& path, AppConfig& config);
    static bool save(const std::string& path, const AppConfig& config);
};
```

Переменные окружения `DDOS_DB_USER` и `DDOS_DB_PASS` перекрывают значения из config.json.

---

### `AppLogger`

**Файл:** [`src/common/AppLogger.hpp`](../src/common/AppLogger.hpp) / [`AppLogger.cpp`](../src/common/AppLogger.cpp)

Тонкая обёртка над `spdlog`.

```cpp
class AppLogger {
public:
    static void init(const std::string& logFile);
    static std::shared_ptr<spdlog::logger> get();
};
```

**Использование:**
```cpp
AppLogger::get()->info("Collector started on interface: {}", ifaceName);
AppLogger::get()->warn("Queue almost full: {} packets", queueSize);
AppLogger::get()->error("DB connection failed: {}", e.what());
```

Лог-файлы: `ddos_collector.log` и `ddos_monitor.log` в рабочей директории.

---

## Модуль `monitor_ui/`

### `DashboardWidget`

**Файл:** [`src/monitor_ui/DashboardWidget.hpp`](../src/monitor_ui/DashboardWidget.hpp)

Главный виджет дашборда. Содержит:
- Главный PPS-график с наложенными слоями (TCP, UDP, ICMP, confidence).
- 3 donut-диаграммы: CPU%, RAM%, соотношение attack/normal.
- Вкладка Deep Analytics: топология сети, тепловая карта портов, SLO Alert Grid, топ source/destination IP.

```cpp
class DashboardWidget : public QWidget {
Q_OBJECT
public slots:
    void updateRealtime(const DetectionResult& result, uint64_t totalPackets);
    void updateSnapshot(float pps, uint64_t totalPackets, int currentLabel);
    void updateConnectionStatus(bool connected);
    void loadHistory(const std::vector<DetectionResult>& events);
    void applyTheme(ThemeMode mode);

signals:
    void bpfToggled(bool enabled);
    void pcapToggled(bool enabled);
};
```

**Вложенные виджеты:**
- `InteractiveChartView` — QChartView с зумом (колесо мыши) и паном (drag).
- `SmartTooltip` — всплывающая подсказка при наведении на график.
- `AlertGridWidget` — сетка 4×8 для SLO-мониторинга.
- `NetworkTopologyWidget` — визуализация источников атак.
- `HeatmapWidget` — тепловая карта активности портов во времени.
- `DonutChartView` — круговые диаграммы с текстом по центру.

---

### `ThemePalette`

**Файл:** [`src/monitor_ui/ThemePalette.hpp`](../src/monitor_ui/ThemePalette.hpp) / [`ThemePalette.cpp`](../src/monitor_ui/ThemePalette.cpp)

Система тем оформления (Catppuccin Mocha / Latte).

```cpp
enum class ThemeMode { Dark, Light };

class ThemePalette {
public:
    static void apply(ThemeMode mode); // Применить QSS к qApp

    // Цвета Catppuccin Mocha (Dark)
    static QColor green();   // #a6e3a1
    static QColor blue();    // #89b4fa
    static QColor red();     // #f38ba8
    static QColor peach();   // #fab387
    static QColor mauve();   // #cba6f7
    static QColor yellow();  // #f9e2af
    static QColor surface(); // #313244
    static QColor base();    // #1e1e2e
    static QColor text();    // #cdd6f4
};
```

---

### `EventHistoryWidget`

**Файл:** [`src/monitor_ui/EventHistoryWidget.hpp`](../src/monitor_ui/EventHistoryWidget.hpp)

Хронология инцидентов безопасности. Загружает данные из `security_events` таблицы PostgreSQL.

---

### `LogWidget`

**Файл:** [`src/monitor_ui/LogWidget.hpp`](../src/monitor_ui/LogWidget.hpp)

Журнал событий — таблица всех DetectionResult окон (label, confidence, PPS, timestamp).

---

### `SessionWidget`

**Файл:** [`src/monitor_ui/SessionWidget.hpp`](../src/monitor_ui/SessionWidget.hpp)

Список сессий из PostgreSQL с кликом для загрузки исторических данных.

---

### `ReportGenerator`

**Файл:** [`src/monitor_ui/ReportGenerator.hpp`](../src/monitor_ui/ReportGenerator.hpp)

Экспорт данных в CSV (с санитизацией через `CSVUtils`).
