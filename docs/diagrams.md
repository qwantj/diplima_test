# Графические материалы (Диаграммы ВКР)

Версия: 2.3  
Дата актуализации: 10.05.2026

Этот документ содержит 5 обязательных диаграмм, требуемых заданием на ВКР, представленных в формате Mermaid.

---

## 1. Структурная схема программного комплекса

Отображает основные компоненты системы и их взаимосвязи.

```mermaid
graph TB
    subgraph "ddos_collector.exe (Сбор и Анализ)"
        A[TrafficMonitor<br/>Npcap / PcapPlusPlus] --> B[FeatureExtractor]
        B --> C[ModelInferencer<br/>ONNX Runtime]
        C --> D[TcpServer<br/>Real-time Stream]
        C --> E[DatabaseManager<br/>PostgreSQL Writer]
    end
    
    subgraph "PostgreSQL"
        DB[(ddos_detection_db)]
    end

    subgraph "ddos_monitor.exe (Визуализация)"
        F[TcpClient] --> G[DataBridge]
        H[DatabaseReader] --> G
        G --> I[Dashboard UI]
        G --> J[Event Log UI]
    end
    
    D -->|"JSON/TCP"| F
    E -->|"Batch INSERT"| DB
    DB -->|"SELECT History"| H
```

---

## 2. Схема алгоритма классификации сетевого трафика

Описывает процесс от захвата пакета до получения вердикта.

```mermaid
flowchart TD
    Start([Начало окна 2 сек]) --> Capture[Захват пакетов из очереди]
    Capture --> Agg[Агрегация статистики: PPS, Bytes, Flow Duration]
    Agg --> Extract[Извлечение 8 признаков CIC-DDoS]
    Extract --> Scale[Нормализация через StandardScaler]
    Scale --> ONNX[Инференс в ONNX Runtime]
    ONNX --> Verdict{Confidence > 50%?}
    Verdict -- Да --> Attack[Пометка: ATTACK]
    Verdict -- Нет --> Benign[Пометка: BENIGN]
    Attack --> Notify[Отправка в Monitor и DB]
    Benign --> Notify
    Notify --> End([Конец окна])
```

---

## 3. ER-диаграмма (схема) базы данных

Описывает структуру таблиц в PostgreSQL.

```mermaid
erDiagram
    SESSIONS ||--o{ EVENTS : "contains"
    SESSIONS ||--o{ STATS_SNAPSHOTS : "has"
    SESSIONS {
        int id PK
        string interface_name
        string model_name
        timestamp start_time
        timestamp end_time
        bigint total_packets
    }
    EVENTS {
        int id PK
        int session_id FK
        timestamp timestamp
        int label
        float confidence
        float pps
        jsonb features
    }
    STATS_SNAPSHOTS {
        int id PK
        int session_id FK
        timestamp timestamp
        float packets_per_s
        bigint total_packets
        int current_label
    }
    SECURITY_EVENTS {
        int id PK
        timestamp start_time
        float duration_sec
        string attacker_ip
        float pps_max
    }
```

---

## 4. Схема взаимодействия программ

Последовательность действий при обнаружении атаки.

```mermaid
sequenceDiagram
    participant C as ddos_collector
    participant D as PostgreSQL
    participant M as ddos_monitor

    C->>C: Захват и Анализ окна
    Note over C: Обнаружена DDoS атака
    par Отправка в реальном времени
        C->>M: JSON {"type":"stats", "label":1, ...}
    and Сохранение в базу
        C->>D: INSERT INTO events (...)
    end
    M->>M: Обновление графиков (Красная зона)
    M->>M: Системное уведомление (Tray)
```

---

## 5. Схема интерфейса пользователя (Формы)

Логическая компоновка основных элементов управления.

```mermaid
graph TD
    subgraph "MainWindow (Qt)"
        T[Toolbar: Load PCAP / Model Selection / Theme]
        
        subgraph "Sidebar"
            S1[Dashboard]
            S2[Analytics]
            S3[Event Log]
            S4[Incidents]
        end
        
        subgraph "Workspace"
            W1[PPS Area Chart]
            W2[System Donuts: CPU/RAM]
            W3[Traffic Table: Top IPs]
        end
    end
    
    T --- Workspace
    Sidebar --- Workspace
```

---

## Инструкция по экспорту
Для использования этих диаграмм в тексте ВКР рекомендуется:
1. Скопировать код Mermaid.
2. Вставить на сайте [Mermaid.live](https://mermaid.live).
3. Скачать в формате PNG (высокое разрешение) или SVG.
