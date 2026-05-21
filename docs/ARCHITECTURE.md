# Архитектура системы DDoS Detection System

**Версия:** 2.2  
**Дата актуализации:** 21.05.2026

---

## 1. Обзор

DDoS Detection System реализована в виде двух отдельных исполняемых файлов, взаимодействующих по TCP, и общей статической библиотеки `ddos_core`.

```
┌──────────────────────────────────────────────────────────────────────┐
│                         ddos_core (статическая библиотека)           │
│  ┌──────────┐ ┌─────────────────┐ ┌────────────────┐ ┌───────────┐  │
│  │ network/ │ │     core/       │ │     ml/        │ │ common/   │  │
│  │TrafficMon│ │DetectionEngine  │ │ModelInferencer │ │Protocol   │  │
│  │FeatureExt│ │FirewallManager  │ │                │ │DatabaseMgr│  │
│  │PcapDumper│ │                 │ │                │ │TcpServer  │  │
│  └──────────┘ └─────────────────┘ └────────────────┘ │TcpClient  │  │
│                                                        │ConfigMgr  │  │
│                                                        │AppLogger  │  │
│                                                        └───────────┘  │
└──────────────────────────────────────────────────────────────────────┘
         ▲                                      ▲
         │ links                                │ links
┌────────┴──────────┐                 ┌─────────┴────────────────────┐
│  ddos_collector   │   TCP:50050     │       ddos_monitor            │
│  (консоль)        │◄───────────────►│  (Qt 6 GUI)                  │
│                   │                 │                               │
│ collector_main.cpp│                 │ monitor_main.cpp              │
│                   │                 │ monitor_ui/DashboardWidget    │
│                   │                 │ monitor_ui/LogWidget          │
│                   │                 │ monitor_ui/EventHistoryWidget │
│                   │                 │ monitor_ui/SessionWidget      │
└──────────┬────────┘                 └──────────┬────────────────────┘
           │ libpqxx                              │ libpqxx
           └──────────────┬───────────────────────┘
                          ▼
                ┌──────────────────┐
                │  PostgreSQL DB   │
                │  (ddos_detection_db)│
                └──────────────────┘
```

---

## 2. Компоненты системы

### 2.1 ddos_collector

Консольное приложение на базе `QCoreApplication`. Выполняет захват и ML-анализ трафика. Является сервером для монитора.

**Жизненный цикл:**
1. Чтение конфигурации (`config.json`, env vars).
2. Инициализация `DetectionEngine` (загрузка модели + скейлера).
3. Запуск `TcpServer` на порту 50050.
4. Подключение к PostgreSQL (`DatabaseManager`).
5. Создание сессии в БД.
6. Запуск захвата (`startLive`) или воспроизведения (`startReplay`).
7. Цикл обработки результатов → TCP broadcast + запись в БД.
8. При получении команды `stop` — корректное завершение, закрытие сессии.

### 2.2 ddos_monitor

GUI-приложение на базе `QApplication`. Подключается к коллектору и отображает данные в реальном времени.

**Основные элементы:**
- `MainWindow` — главное окно с sidebar-навигацией.
- `DataBridge` — мост данных (TCP-клиент + DatabaseManager).
- `DashboardWidget` — основной экран с графиками PPS/протоколов.
- `LogWidget` — журнал классифицированных событий.
- `EventHistoryWidget` — хронология инцидентов безопасности.
- `SessionWidget` — список сессий из PostgreSQL.

### 2.3 ddos_core (статическая библиотека)

Содержит всю бизнес-логику, доступную обоим исполняемым файлам. Собирается как `add_library(ddos_core STATIC ...)`.

---

## 3. Поток данных (Data Flow)

### 3.1 Режим реального времени (Live)

```
Сетевой интерфейс
       │ Npcap raw packets
       ▼
TrafficMonitor::onPacketArrived()
       │ PacketBuffer* (пул буферов)
       ▼
moodycamel::BlockingConcurrentQueue   ← lock-free, до 500 000 пакетов
       │ dequeuePacket()
       ▼
DetectionEngine::inferenceLoop()
       │ каждые 2 секунды:
       ▼
FeatureExtractor::computeNormalizedFeatures()   → вектор 8 признаков
       │
       ▼
ModelInferencer::predict(features)    → {label: 0/1, confidence: 0.0..1.0}
       │
       ▼
DetectionEngine::processWindow()
       │ ┌──────────────────────┐
       │ │ label == 1?           │
       │ │ FirewallManager::     │
       │ │ blockIp(topTalker)    │
       │ └──────────────────────┘
       │
       ├──→ TcpServer::broadcast(stats msg)     → ddos_monitor
       │
       └──→ DatabaseManager::enqueueEvent()     → PostgreSQL
            DatabaseManager::enqueueSnapshot()  → PostgreSQL
```

### 3.2 Режим воспроизведения (Replay)

```
PCAP-файл (*.pcap/*.pcapng)
       │
       ▼
TrafficMonitor::replayPcap()     ← отдельный поток
       │ PacketBuffer* (поштучно с сохранением временны́х меток)
       ▼
[та же цепочка: FeatureExtractor → ModelInferencer]
       │
       ▼
TcpServer::broadcast()          → монитор обновляет графики
       │
       ▼
DatabaseManager                 → отдельная сессия pcap:<path>
       │ notify: "replay_done"
       ▼
monitor: очищает графики, уведомление в трее
```

### 3.3 Поток данных (Monitor ← Collector, команды)

```
monitor GUI action
       │ nlohmann::json
       ▼
TcpClient::sendCommand(cmd, data)
       │ TCP:50050
       ▼
TcpServer::commandReceived(cmd, data)
       │
       ├── "load_pcap"   → DetectionEngine::startReplay()
       ├── "load_model"  → DetectionEngine::hotSwapModel()
       ├── "config_dump" → DetectionEngine::enablePcapDump()
       └── "stop"        → QCoreApplication::quit()
```

---

## 4. Многопоточность

Система намеренно использует минимальное число потоков с lock-free взаимодействием.

| Поток | Компонент | Описание |
|---|---|---|
| Главный | `QCoreApplication::exec()` | Qt event loop, TcpServer, таймеры |
| Capture | `TrafficMonitor` (Npcap callback) | Захват пакетов, помещение в очередь |
| Inference | `DetectionEngine::inferenceThread_` | Чтение очереди, вычисление признаков, ML, колбэки |
| DB Writer | `DatabaseManager::writerThread_` | Асинхронная запись в PostgreSQL каждые 5 сек |
| Replay | `TrafficMonitor::replayPcap()` | Чтение PCAP-файла (только в режиме replay) |

**Lock-free коммуникация:**  
Пакеты передаются между потоками через `moodycamel::BlockingConcurrentQueue<PacketBuffer*>`. Пул буферов реализован через `moodycamel::ConcurrentQueue<PacketBuffer*>` для zero-copy переиспользования памяти.

**Горячая замена модели (Hot Swap):**  
`ModelInferencer` использует `std::shared_mutex` — `hotSwapModel()` захватывает exclusive lock, а `predict()` — shared lock. Это позволяет переключать модель на лету без остановки коллектора.

---

## 5. Обработка ошибок и устойчивость

- **Нет БД:** коллектор продолжает работу без persistence. Детекция не прерывается.
- **Разрыв TCP:** монитор автоматически переподключается через `TcpClient`. Пока монитор отключён, `FileBuffer` аккумулирует данные в `pending_events.jsonl` и отправляет их пакетом при восстановлении соединения.
- **Перегрузка очереди:** при заполнении очереди пакетов (>500k) счётчик `droppedPackets_` инкрементируется — логика захвата не блокируется (load shedding).
- **Исключения БД:** все операции с pqxx обёрнуты в `try/catch`. При ошибке асинхронный writer делает повторную попытку на следующем тике (каждые 5 сек).

---

## 6. Схема протокола обмена (упрощённо)

```
Collector → Monitor:
  stats    [каждые 2 сек]  — полная телеметрия окна
  snapshot [каждые 2 сек]  — лёгкий снимок (pps, label)
  notify                   — события: replay_started/done, live_resumed

Monitor → Collector (CMD):
  load_pcap / stop_replay
  load_model
  config_bpf
  config_dump
  stop
```

Полный протокол: [API_REFERENCE.md](API_REFERENCE.md).

---

## 7. Зависимости

| Библиотека | Версия | Источник | Назначение |
|---|---|---|---|
| Qt 6 | 6.6+ | Qt Installer | GUI, сигналы/слоты, сеть |
| PcapPlusPlus | актуальная | vcpkg | Захват пакетов (Npcap) |
| libpqxx | 7.x | vcpkg | Клиент PostgreSQL |
| PostgreSQL | 14+ | vcpkg | Клиентские заголовки/lib |
| ONNX Runtime | 1.18+ | manual | Инференс ML-моделей |
| spdlog | 1.15.2 | FetchContent | Структурированное логирование |
| nlohmann/json | 3.11.3 | FetchContent | JSON сериализация |
| moodycamel/concurrentqueue | — | bundled | Lock-free очередь пакетов |

---

## 8. Производительность

- **Пропускная способность:** до 130 000+ пакетов/сек на стандартном ПК (Core i7, 16GB RAM).
- **Задержка инференса:** < 1 мс для RF, < 0.5 мс для MLP (CPU mode).
- **Окно инференса:** 2 секунды (configurable через `config.json` → `ml.window_sec`).
- **Очередь:** до 500 000 пакетов в памяти (`network.max_queue_size`).
- **Запись в БД:** асинхронная, батчами через `COPY` (pqxx::stream_to), не блокирует inference поток.

---

## 9. Ограничения текущей реализации

- Ориентирована на **Windows x64 / MSVC 2022**.
- Не поддерживает распределённую установку (collector и monitor на разных хостах — теоретически поддерживается, но не тестировалось).
- ML-модели обучены на датасете CICIDS — возможен дрейф распределений на нетипичном трафике.
- `FirewallManager` использует `netsh advfirewall` — требует прав администратора.
