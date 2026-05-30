# Схема базы данных PostgreSQL

**Версия:** 2.2  
**Дата актуализации:** 21.05.2026  
**СУБД:** PostgreSQL 14+  
**База данных:** `ddos_detection_db`

---

## Обзор

База данных содержит 4 таблицы, создаваемые автоматически при первом запуске `ddos_collector` (метод `DatabaseManager::ensureTables()`).

```
sessions
   │ (1:N)
   ├──→ events
   ├──→ stats_snapshots
   └──→ security_events
```

---

## Таблица `sessions`

Хранит информацию о каждом сеансе работы системы.

```sql
CREATE TABLE IF NOT EXISTS sessions (
    id              SERIAL PRIMARY KEY,
    interface_name  TEXT,               -- Имя интерфейса или 'pcap:<path>'
    model_name      TEXT,               -- Имя загруженной ONNX-модели
    start_time      TIMESTAMP DEFAULT NOW(),
    end_time        TIMESTAMP,          -- NULL если сессия активна
    total_packets   BIGINT DEFAULT 0,   -- Всего обработано пакетов
    total_attacks   BIGINT DEFAULT 0,   -- Количество окон с label=1 (атака)
    total_benign    BIGINT DEFAULT 0    -- Количество окон с label=0 (норма)
);
```

**Жизненный цикл записи:**
- **CREATE:** при старте коллектора → `DatabaseManager::createSession(iface, modelName)`
- **UPDATE (close):** при завершении коллектора → `DatabaseManager::closeSession(id, total, attacks, benign)`

**Режим PCAP replay:** создаётся отдельная сессия с `interface_name = "pcap:<полный_путь>"`.

---

## Таблица `events`

Хранит результат каждого окна детекции (одна запись каждые 1 секунда при активном трафике).

```sql
CREATE TABLE IF NOT EXISTS events (
    id              SERIAL PRIMARY KEY,
    session_id      INT REFERENCES sessions(id),
    timestamp       TIMESTAMP DEFAULT NOW(),
    label           INT,            -- 0 = benign, 1 = attack
    confidence      REAL,           -- Вероятность предсказания [0.0..1.0]
    pps             REAL,           -- Пакетов в секунду в этом окне
    total_packets   BIGINT,         -- Всего пакетов за сессию на момент окна
    features        JSONB,          -- JSON-массив из 8 нормализованных признаков
    model_name      TEXT            -- Имя модели, выдавшей предсказание
);
```

**Признаки в поле `features` (JSONB-массив):**

| Индекс | Название | Описание |
|---|---|---|
| 0 | Flow Duration | Длительность потока, мкс |
| 1 | Total Fwd Packets | Число прямых пакетов |
| 2 | Total Backward Packets | Число обратных пакетов |
| 3 | Total Length of Fwd Packets | Суммарная длина прямых пакетов, байт |
| 4 | Total Length of Bwd Packets | Суммарная длина обратных пакетов, байт |
| 5 | Fwd Packet Length Mean | Средняя длина прямого пакета |
| 6 | Bwd Packet Length Mean | Средняя длина обратного пакета |
| 7 | Flow Packets/s | Пакетов в секунду для потока |

**Запись:** асинхронно, батчами по ≤1000 записей через `pqxx::stream_to` (PostgreSQL COPY) каждые 5 секунд.

---

## Таблица `stats_snapshots`

Лёгкие периодические снимки состояния системы для построения графиков PPS.

```sql
CREATE TABLE IF NOT EXISTS stats_snapshots (
    id              SERIAL PRIMARY KEY,
    session_id      INT REFERENCES sessions(id),
    timestamp       TIMESTAMP DEFAULT NOW(),
    packets_per_s   REAL,           -- PPS в момент снимка
    total_packets   BIGINT,         -- Накопленное кол-во пакетов
    current_label   INT             -- 0 = норма, 1 = атака
);
```

**Назначение:** используется `DashboardWidget` для загрузки исторических данных PPS-графика при переключении между сессиями.

---

## Таблица `security_events`

Агрегированные инциденты безопасности. Одна запись = один непрерывный период атаки.

```sql
CREATE TABLE IF NOT EXISTS security_events (
    id              SERIAL PRIMARY KEY,
    session_id      INT REFERENCES sessions(id),
    start_time      TIMESTAMP,      -- Момент начала атаки
    duration_sec    REAL,           -- Продолжительность, секунды
    attacker_ip     TEXT,           -- IP с наибольшим числом пакетов атаки
    pps_max         REAL,           -- Пиковый PPS за всё время инцидента
    type_label      TEXT,           -- Тип: "DDoS Attack"
    confidence      REAL            -- Средняя уверенность модели
);
```

**Логика агрегации** (в `collector_main.cpp`):
- Переход `label=0 → label=1` при `confidence > 0.4`: открывается новый инцидент.
- Пока `label=1`: обновляются `maxPps`, `totalConf`, `samples`.
- Переход `label=1 → label=0`: инцидент закрывается и записывается.

---

## SQL-запросы (часто используемые)

### Просмотр последних 10 сессий
```sql
SELECT id, interface_name, model_name, start_time, end_time,
       total_packets, total_attacks, total_benign
FROM sessions
ORDER BY id DESC
LIMIT 10;
```

### Получить все события атак для сессии N
```sql
SELECT timestamp, label, confidence, pps, features
FROM events
WHERE session_id = N AND label = 1
ORDER BY timestamp;
```

### Статистика атак по сессиям
```sql
SELECT s.id, s.interface_name, s.model_name,
       COUNT(e.id) as attack_windows,
       MAX(e.pps) as max_pps,
       AVG(e.confidence) as avg_confidence
FROM sessions s
LEFT JOIN events e ON e.session_id = s.id AND e.label = 1
GROUP BY s.id
ORDER BY s.id DESC;
```

### Инциденты безопасности за последние 24 часа
```sql
SELECT start_time, duration_sec, attacker_ip, pps_max, type_label, confidence
FROM security_events
WHERE start_time > NOW() - INTERVAL '24 hours'
ORDER BY start_time DESC;
```

### Топ атакующих IP за всё время
```sql
SELECT attacker_ip, COUNT(*) as incident_count, MAX(pps_max) as max_pps
FROM security_events
WHERE attacker_ip != 'unknown'
GROUP BY attacker_ip
ORDER BY incident_count DESC
LIMIT 20;
```

---

## Управление данными

### Создание базы данных
```sql
CREATE DATABASE ddos_detection_db;
```

### Очистка старых данных (ротация)
```sql
-- Удалить данные сессий старше 30 дней
DELETE FROM events e
USING sessions s
WHERE e.session_id = s.id AND s.start_time < NOW() - INTERVAL '30 days';

DELETE FROM stats_snapshots ss
USING sessions s
WHERE ss.session_id = s.id AND s.start_time < NOW() - INTERVAL '30 days';

DELETE FROM security_events se
USING sessions s
WHERE se.session_id = s.id AND s.start_time < NOW() - INTERVAL '30 days';

DELETE FROM sessions WHERE start_time < NOW() - INTERVAL '30 days';
```

### Индексы для ускорения запросов (рекомендуется)
```sql
CREATE INDEX IF NOT EXISTS idx_events_session_id ON events(session_id);
CREATE INDEX IF NOT EXISTS idx_events_timestamp ON events(timestamp);
CREATE INDEX IF NOT EXISTS idx_snapshots_session_id ON stats_snapshots(session_id);
CREATE INDEX IF NOT EXISTS idx_security_events_start ON security_events(start_time);
```

---

## Конфигурация подключения

Параметры подключения задаются в `config.json` или через переменные окружения:

| Параметр | config.json | Переменная окружения |
|---|---|---|
| Хост | `database.host` | — |
| Порт | `database.port` | — |
| База | `database.name` | — |
| Пользователь | `database.user` | `DDOS_DB_USER` |
| Пароль | `database.password` | `DDOS_DB_PASS` |

**Приоритет:** переменные окружения > config.json.

---

## Асинхронная запись (DatabaseManager)

`DatabaseManager` поддерживает **два независимых соединения** с PostgreSQL:

1. **`conn_`** (главный поток) — для `createSession`, `closeSession`, `getSessions`, `getEventsForSession`.
2. **`writerConn_`** (поток писателя `writerThread_`) — для асинхронного батч-флаша событий.

**Очереди (lock-free):**
- `eventQueue_` — очередь `EventEntry` (DetectionResult)
- `snapshotQueue_` — очередь `SnapshotEntry` (pps, label)
- `securityEventQueue_` — очередь `SecurityEventEntry` (инциденты)

**Флаш** происходит каждые **5000 мс** (`FLUSH_INTERVAL_MS`) через `QTimer` в потоке писателя. При остановке системы выполняется финальный флаш на главном соединении.

**Буфер отказоустойчивости:** при недоступности БД события сохраняются в `pending_events.jsonl` (`FileBuffer`) и загружаются при восстановлении соединения.
