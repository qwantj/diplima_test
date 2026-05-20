# Полная техническая документация

  

Версия: 2.4  
Дата актуализации: 20.05.2026
  

## 1. Назначение комплекса

  

DDoS Detection System — программный комплекс для мониторинга сетевой активности и обнаружения аномалий типа отказ в обслуживании.

  

Состав комплекса:

  

1. Программа сбора и анализа данных: ddos_collector.

2. Программа мониторинга и управления: ddos_monitor.

3. Реляционная база данных: PostgreSQL.

  

## 2. Покрытие требований задания ВКР

  

| Требование | Реализация |

|---|---|

| Захват трафика в режиме реального времени | TrafficMonitor (PcapPlusPlus, Npcap) |

| Загрузка и анализ дампов | --pcap, команда load_pcap, replay loop |

| Извлечение признаков | FeatureExtractor (16 признаков окна и потока) |

| ML-классификация | ModelInferencer + ONNX Runtime |

| Активное противодействие | FirewallManager (блокировка атакующих IP) |

| Сохранение событий в БД | DatabaseManager (async batch writer) |

| Визуализация и управление | Dashboard/Log/Sessions/EventHistory в Qt GUI |

  

## 3. Архитектура

  

### 3.1 Слои

  

- network: захват и разбор пакетов (с поддержкой JSON-маппинга признаков);

- core: orchestration, inference window и управление блокировками (FirewallManager);

- ml: загрузка модели, predict и thread-safe горячая замена (hot swap);

- common: протокол, TCP, БД, логирование, управление конфигурацией (ConfigManager), сбор системных метрик;

- monitor_ui: масштабируемая визуализация (вкл. Heatmap, Topology, Сетка алертов) и системный трей.

  

### 3.2 Исполняемые файлы

  

- ddos_collector.exe

- ddos_monitor.exe

  

Общее ядро собирается в static library ddos_core.

  

## 4. Потоки данных

  

### 4.1 Режим реального времени

  

1. collector захватывает пакеты с интерфейса.

2. пакеты помещаются в lock-free очередь.

3. каждые 2 секунды рассчитывается окно признаков (16 параметров).

4. ONNX-модель выдает label и confidence.

5. результат отправляется:

   - в monitor по TCP;

   - в PostgreSQL (events/stats/security_events).

6. При обнаружении атаки и наличии соответствующих настроек, FirewallManager блокирует IP источника.

  

### 4.2 Режим воспроизведения

  

1. запуск через --pcap или GUI-команду load_pcap.

2. collector читает pcap и формирует окна по временным меткам.

3. после завершения отправляется notify: replay_done.

  

## 5. Протокол обмена в реальном времени

  

Формат: JSON lines (одно сообщение на строку) поверх TCP.

  

### 5.1 Основные типы сообщений

  

- stats: полная телеметрия окна (pps, label, confidence, топы, features, протоколы, заблокированные IP);

- snapshot: облегченный периодический снимок для быстрого обновления GUI;

- cmd: команда monitor -> collector;

- notify: уведомление collector -> monitor;

- buffer: пачка сообщений.

  

### 5.2 Команды управления

  

- load_pcap / stop_replay

- load_model

- config_bpf

- config_update (обновление настроек на лету)

- stop (завершение работы коллектора)

  

## 6. Модель данных PostgreSQL

  

Используемые таблицы:

  

- sessions

- events

- stats_snapshots

- security_events

  

### 6.1 Таблица sessions

  

Хранит жизненный цикл сессии:

  

- interface

- model_name

- start_time/end_time

- total_packets/total_attacks/total_benign

  

### 6.2 Таблица events

  

Хранит результаты окон детекции:

  

- timestamp

- label/confidence

- packets_per_s

- признаки окна (8 или более полей)

  

### 6.3 Таблица stats_snapshots

  

Периодические снимки:

  

- packets_per_s

- total_packets

- current_label

  

### 6.4 Таблица security_events

  

Агрегированные инциденты атаки:

  

- start_time

- duration_sec

- attacker_ip

- pps_max

- type_label

- confidence

  

## 7. Ключевые параметры времени выполнения

  

- Окно инференса: 2 секунды.

- MAX_QUEUE_SIZE: 500000.

- Flush-интервал БД: 5000 мс.

- TCP-порт по умолчанию: 50050.

- Файл конфигурации: config.json.

  

## 8. Параметры командной строки ddos_collector

  

- --list-interfaces

- -i, --interface <name>

- -f, --pcap <file>

- -m, --model <path>

- -e, --ep <cpu|cuda|dml>

- --tcp-port <port>

- --db-host, --db-port, --db-name, --db-user, --db-password

- --pcap-dir <dir>

  

## 9. Графический интерфейс ddos_monitor

  

- Подключение к collector по TCP.

- Подключение к PostgreSQL для истории.

- Открытие pcap из интерфейса и горячая замена модели (hot swap).

- **Масштабный Dashboard:**

  - раздельные сглаженные графики потоков (TCP, UDP, ICMP);

  - тепловые карты распределения портов (Heatmap) и топология;

  - сетка мониторинга инцидентов (SLO Alert Grid);

  - мониторинг системных ресурсов (RAM, CPU, загрузка канала).

- Журнал событий с оптимизированной мягкой цветовой палитрой для снижения утомляемости.

- Системный трей (SysTray) для фоновой индикации уровня угрозы.

- Переключение темы (System/Dark/Light).

- Экспорт данных в CSV с защитой от инъекций (CSVUtils).

  

## 10. Нефункциональные свойства

  

- Высокая пропускная способность за счет lock-free очереди.

- Устойчивость к перегрузке через load shedding.

- Асинхронная запись в БД снижает блокировки потока реального времени.

- Потокобезопасная (thread-safe) архитектура при горячей замене моделей ML.

- Возможность работы без БД (collector продолжает детектирование, но без persistence).

- Настраиваемый макет (layout) для гибкой интеграции новых виджетов.

  

## 11. Ограничения

  

- Текущая реализация ориентирована на Windows/MSVC.

- Признаки агрегируются по окну интерфейса и flow-based анализу.

- Качество классификации зависит от соответствия train/live распределений.

  

## 12. Связанные документы

  

- docs/Codebase.md

- docs/FeatureExtractionAlgorithm.md

- docs/diagrams.md

- README.md

- TROUBLESHOOTING.md

- PZ_SUMMARY_RU.md