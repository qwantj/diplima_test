# Полная техническая документация

**Версия:** 2.2  
**Дата актуализации:** 21.05.2026

---

## 1. Назначение комплекса

DDoS Detection System — программный комплекс для мониторинга сетевой активности и обнаружения аномалий типа «отказ в обслуживании» (DoS/DDoS).

Состав комплекса:
1. **Программа сбора и анализа данных:** `ddos_collector.exe` (консоль)
2. **Программа мониторинга и управления:** `ddos_monitor.exe` (Qt 6 GUI)
3. **Общая статическая библиотека:** `ddos_core.lib`
4. **Реляционная база данных:** PostgreSQL 14+

---

## 2. Покрытие требований задания ВКР

| Требование | Реализация | Файл |
|---|---|---|
| Захват трафика в реальном времени | TrafficMonitor (PcapPlusPlus, Npcap) | `network/TrafficMonitor.cpp` |
| Загрузка и анализ дампов | `--pcap`, команда `load_pcap`, replay loop | `collector_main.cpp` |
| Извлечение признаков | FeatureExtractor (8 признаков CICIDS) | `network/FeatureExtractor.cpp` |
| Нормализация признаков | StandardScaler (mean/scale JSON) | `network/FeatureExtractor.cpp` |
| ML-классификация | ModelInferencer + ONNX Runtime | `ml/ModelInferencer.cpp` |
| Горячая замена модели | `hotSwapModel()` (shared_mutex) | `core/DetectionEngine.cpp` |
| Активное противодействие | FirewallManager (`netsh advfirewall`) | `core/FirewallManager.cpp` |
| Сохранение событий в БД | DatabaseManager (async batch COPY) | `common/DatabaseManager.cpp` |
| Визуализация PPS-графиков | DashboardWidget (QChart, QAreaSeries) | `monitor_ui/DashboardWidget.cpp` |
| Тепловые карты | HeatmapWidget (Custom QPainter) | `monitor_ui/DashboardWidget.cpp` |
| Топология сети | NetworkTopologyWidget | `monitor_ui/DashboardWidget.cpp` |
| История сессий и событий | SessionWidget, EventHistoryWidget | `monitor_ui/` |
| Экспорт данных | ReportGenerator + CSVUtils | `monitor_ui/ReportGenerator.cpp` |
| Системный трей | QSystemTrayIcon + уведомления | `monitor_main.cpp` |

---

## 3. Архитектура

### 3.1 Слои

- **network/** — захват и разбор пакетов (PcapPlusPlus/Npcap), flow-based агрегация признаков, запись PCAP-дампов.
- **core/** — оркестрация детекции (inference window loop), управление блокировками (FirewallManager), отслеживание инцидентов.
- **ml/** — загрузка ONNX-модели, предсказание, потокобезопасная горячая замена.
- **common/** — протокол обмена (TCP + JSON), PostgreSQL клиент, конфигурация, логирование (spdlog), сборщик системных метрик, утилиты.
- **monitor_ui/** — Qt 6 GUI с масштабируемым Dashboard, системой тем (Catppuccin Mocha/Latte).

### 3.2 Исполняемые файлы

- `ddos_collector.exe` — консоль, работает в фоне, требует прав администратора (Npcap + FirewallManager).
- `ddos_monitor.exe` — GUI, может работать от обычного пользователя (БД-подключение идёт через DataBridge).

Общее ядро: статическая библиотека `ddos_core.lib`.

### 3.3 Межпроцессное взаимодействие

TCP-сокет (порт 50050 по умолчанию). Протокол: JSON + 4-байтовый заголовок длины (length-prefixed framing). Версия протокола: `2.2`.

---

## 4. Потоки данных

### 4.1 Режим реального времени

1. `TrafficMonitor` захватывает пакеты с сетевого интерфейса (callback Npcap).
2. Пакеты помещаются в lock-free очередь (`moodycamel::BlockingConcurrentQueue`, до 500 000 пакетов).
3. `DetectionEngine::inferenceLoop()` читает пакеты, передаёт в `FeatureExtractor::processPacket()`.
4. Каждые 2 секунды вычисляется вектор из 8 нормализованных признаков.
5. `ModelInferencer::predict()` выдаёт `{label: 0/1, confidence: 0.0..1.0}`.
6. Результат отправляется:
   - `TcpServer::broadcast(stats)` → `ddos_monitor`.
   - `DatabaseManager::enqueueEvent()` / `enqueueSnapshot()` → PostgreSQL (async).
7. При `label=1` и включённой митигации: `FirewallManager::blockIp(topTalker)`.

### 4.2 Режим воспроизведения (Replay)

1. Запуск через `--pcap <file>` или GUI-команду `load_pcap`.
2. Создаётся отдельная сессия в БД с префиксом `pcap:<path>`.
3. `TrafficMonitor::replayPcap()` читает файл в отдельном потоке.
4. По завершении отправляется `notify: replay_done`.
5. Возврат к live: команда `stop_replay` → `notify: live_resumed`.

---

## 5. Протокол обмена

Формат: JSON + 4-байтовый length-prefix header поверх TCP.

### 5.1 Типы сообщений (Collector → Monitor)

| Тип | Периодичность | Описание |
|---|---|---|
| `stats` | каждые 2 сек | Полная телеметрия: pps, label, confidence, top IPs/ports, features, blocked IPs |
| `snapshot` | каждые 2 сек | Лёгкий снимок: pps, total_packets, current_label |
| `notify` | по событию | Уведомления: replay_started, replay_done, live_resumed |
| `buffer` | при переподключении | Батч накопленных stats-сообщений |

### 5.2 Команды управления (Monitor → Collector)

| Команда | Описание |
|---|---|
| `load_pcap` | Запустить анализ PCAP-файла (поле `path`) |
| `stop_replay` | Прекратить replay, вернуться к live |
| `load_model` | Загрузить новую ONNX-модель (hot swap) |
| `config_bpf` | Включить/выключить BPF-фильтр |
| `config_dump` | Включить/выключить запись PCAP-дампов |
| `config_update` | Обновить конфигурацию на лету |
| `stop` | Завершить работу коллектора |

Полный протокол: [API_REFERENCE.md](API_REFERENCE.md).

---

## 6. Модель данных PostgreSQL

### Схема таблиц

```sql
sessions        -- жизненный цикл сессии (iface, model, times, stats)
  └──► events              -- результаты окон (label, confidence, features JSON)
  └──► stats_snapshots     -- периодические снимки PPS
  └──► security_events     -- агрегированные инциденты атак
```

| Таблица | Назначение | Ключевые поля |
|---|---|---|
| `sessions` | Сессия работы системы | interface_name, model_name, total_attacks |
| `events` | Результат каждого окна | label, confidence, pps, features (JSONB) |
| `stats_snapshots` | Лёгкие снимки для графиков | packets_per_s, current_label |
| `security_events` | Агрегированные инциденты | start_time, duration_sec, attacker_ip, pps_max |

Подробная схема: [DATABASE_SCHEMA.md](DATABASE_SCHEMA.md).

---

## 7. Конфигурация

Файл `config.json` (копируется в директорию сборки):

| Параметр | Описание | По умолчанию |
|---|---|---|
| `collector_host` | IP коллектора (для монитора) | `"127.0.0.1"` |
| `tcp_port` | TCP-порт | `50050` |
| `database.host` | PostgreSQL хост | `"localhost"` |
| `database.port` | PostgreSQL порт | `5432` |
| `database.name` | Имя базы данных | `"ddos_detection_db"` |
| `database.user` | Пользователь БД | `""` |
| `database.password` | Пароль БД | `""` |
| `ml.default_model` | Путь к ONNX-модели | `"models/rf_model.onnx"` |
| `ml.default_ep` | Execution provider | `"cpu"` |
| `ml.window_sec` | Длительность окна инференса | `2.0` |
| `network.default_interface` | Имя интерфейса по умолчанию | `""` |
| `network.max_queue_size` | Макс. размер очереди пакетов | `500000` |

**Переменные окружения** (приоритет выше config.json):
- `DDOS_DB_USER` — логин PostgreSQL
- `DDOS_DB_PASS` — пароль PostgreSQL

---

## 8. Параметры командной строки ddos_collector

| Параметр | Описание |
|---|---|
| `--list-interfaces` | Вывести список доступных интерфейсов |
| `-i <name>`, `--interface <name>` | Имя или IP сетевого интерфейса |
| `-f <file>`, `--pcap <file>` | PCAP-файл для воспроизведения |
| `-m <path>`, `--model <path>` | Путь к ONNX-модели |
| `-e <ep>`, `--ep <ep>` | Execution provider: cpu / cuda / dml |
| `--tcp-host <host>` | Хост TCP-сервера |
| `--tcp-port <port>` | Порт TCP-сервера (по умолчанию 50050) |
| `--db-host`, `--db-port`, `--db-name`, `--db-user`, `--db-password` | Параметры PostgreSQL |
| `--pcap-dir <dir>` | Директория для записи PCAP-дампов |

---

## 9. Графический интерфейс ddos_monitor

**Подключение:** к коллектору по TCP, к PostgreSQL для исторических данных.

**Управление:**
- Открытие PCAP из GUI + горячая замена модели (hot swap).
- Возврат к live-трафику кнопкой ⏹ Live.
- Запись PCAP-дампов кнопкой ⏺ Запись.

**Масштабный Dashboard:**
- PPS-график с раздельными слоями (TCP, UDP, ICMP, confidence), зум/пан.
- Тепловые карты распределения портов во времени (HeatmapWidget).
- Топология сети (NetworkTopologyWidget).
- SLO Alert Grid (4×8, history аптайма).
- Donut-диаграммы CPU%, RAM%, attack ratio.
- Мониторинг системных ресурсов (SystemMetricsCollector).

**Event Log:** журнал всех classified windows с цветовой маркировкой.

**Security Incidents:** агрегированная хронология атак.

**Sessions History:** список сессий, клик загружает исторические данные.

**Системный трей:** фоновая индикация (нормальный / атака), уведомления.

**Темы оформления:** Dark (Catppuccin Mocha) / Light (Catppuccin Latte).

---

## 10. Нефункциональные характеристики

| Характеристика | Значение |
|---|---|
| Пропускная способность | до 130 000+ пакетов/сек |
| Задержка инференса | < 1 мс (RF, CPU mode) |
| Окно инференса | 2 секунды |
| Очередь пакетов | до 500 000 (lock-free) |
| Запись в БД | async, batch COPY, каждые 5 сек |
| Платформа | Windows x64, MSVC 2022 |

**Механизмы устойчивости:**
- **Load shedding:** переполнение очереди не блокирует захват — пакеты отбрасываются со счётчиком.
- **Offline буферизация:** при недоступности БД данные сохраняются в `pending_events.jsonl` и отправляются при восстановлении.
- **Работа без БД:** детекция и TCP-трансляция продолжают работу при отсутствии PostgreSQL.
- **Hot swap модели:** замена ONNX-модели на лету через `shared_mutex` без прерывания захвата.

---

## 11. Ограничения

- Ориентирована на **Windows x64 / MSVC 2022**.
- ML-модели обучены на **CICIDS2017** — возможен дрейф на нетипичном трафике.
- `FirewallManager` требует прав **администратора**.
- Flow-tracking — агрегация по окну (не stateful TCP-сессии).
- Не поддерживает IPv6 в текущей реализации FeatureExtractor.

---

## 12. Связанные документы

| Документ | Описание |
|---|---|
| [ARCHITECTURE.md](ARCHITECTURE.md) | Детальная архитектура и диаграммы потоков данных |
| [API_REFERENCE.md](API_REFERENCE.md) | Протокол обмена Collector↔Monitor |
| [CLASS_REFERENCE.md](CLASS_REFERENCE.md) | Справочник по классам и методам |
| [DATABASE_SCHEMA.md](DATABASE_SCHEMA.md) | Схема PostgreSQL, SQL-запросы |
| [FeatureExtractionAlgorithm.md](FeatureExtractionAlgorithm.md) | Алгоритм извлечения признаков |
| [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) | Руководство разработчика |
| [USER_GUIDE.md](USER_GUIDE.md) | Руководство пользователя |
| [TROUBLESHOOTING.md](TROUBLESHOOTING.md) | Устранение неполадок |
| [windows-setup.md](windows-setup.md) | Установка на Windows |
| [diagrams.md](diagrams.md) | Диаграммы (ВКР) |