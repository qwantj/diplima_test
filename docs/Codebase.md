# Справочник по кодовой базе

  

Дата актуализации: 10.05.2026

  

## 1. Дерево исходников

  

```text

src/

  collector_main.cpp

  monitor_main.cpp

  main.cpp

  concurrentqueue.h

  core/

    DetectionEngine.hpp

    DetectionEngine.cpp

  network/

    TrafficMonitor.hpp

    TrafficMonitor.cpp

    FeatureExtractor.hpp

    FeatureExtractor.cpp

    PcapDumper.hpp

    PcapDumper.cpp

  ml/

    ModelInferencer.hpp

    ModelInferencer.cpp

  common/

    Protocol.hpp

    Protocol.cpp

    TcpServer.hpp

    TcpServer.cpp

    TcpClient.hpp

    TcpClient.cpp

    DataBridge.hpp

    DataBridge.cpp

    DatabaseManager.hpp

    DatabaseManager.cpp

    SystemMetricsCollector.hpp

    SystemMetricsCollector.cpp

    AppLogger.hpp

    AppLogger.cpp

    FileBuffer.hpp

    FileBuffer.cpp

  monitor_ui/

    DashboardWidget.hpp

    DashboardWidget.cpp

    LogWidget.hpp

    LogWidget.cpp

    SessionWidget.hpp

    SessionWidget.cpp

    EventHistoryWidget.hpp

    EventHistoryWidget.cpp

    ThemePalette.hpp

    ThemePalette.cpp

```

  

## 2. Ответственность модулей

  

### 2.1 Модуль ядра (core)

  

DetectionEngine:

  

- инициализирует extractor/inferencer;

- управляет live/replay циклом;

- формирует окна инференса;

- отправляет callbacks для сети и БД.

  

### 2.2 Модуль сетевого захвата (network)

  

TrafficMonitor:

  

- live-захват пакетов;

- list interfaces;

- BPF-фильтр.

  

FeatureExtractor:

  

- сбор статистики окна;

- расчет 8 признаков;

- нормализация через scaler JSON;

- топы IP/портов/целей.

  

PcapDumper:

  

- сохранение размеченных дампов.

  

### 2.3 Модуль машинного обучения (ml)

  

ModelInferencer:

  

- загрузка ONNX;

- выбор execution provider;

- предсказание label + confidence;

- поддержка переключения модели.

  

### 2.4 Общий инфраструктурный модуль (common)

  

Protocol:

  

- сериализация/десериализация JSON-сообщений.

  

TcpServer/TcpClient:

  

- транспорт реального времени между collector и monitor.

  

DataBridge:

  

- фасад в monitor для работы с TCP и БД.

  

DatabaseManager:

  

- подключение к PostgreSQL;

- сессии/события/снимки/инциденты;

- асинхронный пакетный writer.

  

SystemMetricsCollector:

  

- сбор CPU/RAM для телеметрии.

  

AppLogger:

  

- централизованное логирование.

  

### 2.5 Модуль интерфейса монитора (monitor_ui)

  

DashboardWidget:

  

- визуализация в реальном времени, графики (раздельные линии агрегации), summary.

- включает встроенные классы: `HeatmapWidget`, `AlertGridWidget`, `TopologyWidget`, `InteractiveChartView`.

  

LogWidget:

  

- поток событий детекции.

  

SessionWidget:

  

- просмотр прошлых сессий.

  

EventHistoryWidget:

  

- просмотр инцидентов из БД.

  

ThemePalette:

  

- оформление System/Dark/Light.

  

## 3. Точки входа

  

- collector_main.cpp: запуск ddos_collector.

- monitor_main.cpp: запуск ddos_monitor (содержит логику QSystemTrayIcon для фонового отслеживания).

  

## 4. Критичные контракты

  

- Протокол Protocol serialize/deserialize.

- DetectionResult и связанные структуры из DetectionEngine.hpp.

- Согласованность порядка признаков между FeatureExtractor и scaler/model.

  

## 5. Краткая карта зависимостей

  

- collector_main -> DetectionEngine + DatabaseManager + TcpServer

- monitor_main -> DataBridge + monitor_ui widgets

- DetectionEngine -> TrafficMonitor + FeatureExtractor + ModelInferencer + PcapDumper

- DataBridge -> TcpClient + DatabaseManager + Protocol