# Графические материалы к ВКР
## «Разработка программного комплекса обнаружения атак типа «отказ в обслуживании»»

В данном файле приведены описания и исходный код (Mermaid) для 5 обязательных листов графического материала.

---

### Лист 1. Структурная схема программного комплекса

Отражает архитектуру системы, разделение на Collector и Monitor, а также связи с БД и ОС.

```mermaid
graph TD
    subgraph "Узел-генератор (Атакующий)"
        A[hping3 / Scapy]
    end

    subgraph "Защищаемый узел (Target)"
        subgraph "ddos_collector (Ядро)"
            B[Traffic Monitor] --> C[Feature Extractor]
            C --> D[Model Inferencer]
            D --> E[Detection Engine]
            E --> F[Firewall Manager]
        end

        subgraph "Хранилище"
            G[(PostgreSQL)]
        end

        subgraph "ddos_monitor (GUI)"
            H[Data Bridge] --> I[Dashboard]
            H --> J[Event History]
        end

        subgraph "ОС (Windows/Linux)"
            K[Network Interface]
            L[Windows Firewall / iptables]
        end
    end

    A -- "DoS/DDoS Трафик" --> K
    K -- "Raw Packets" --> B
    E -- "Events & Stats" --> G
    G -- "Historical Data" --> H
    E -- "Real-time Metrics (TCP)" --> H
    F -- "Netsh / Shell" --> L
```

---

### Лист 2. Схема алгоритма главного цикла обработки (ddos_collector)

Выполнена в соответствии с ГОСТ 19.701-90.

```mermaid
flowchart TD
    A([Начало]) --> B[Загрузка config.json]
    B --> C[Инициализация БД и Логгера]
    C --> D[Загрузка ML-моделей .onnx]
    D --> E[Открытие сетевого интерфейса]
    E --> F{Пакет получен?}
    F -- Нет --> F
    F -- Да --> G[Извлечение признаков L2-L4]
    G --> H[Нормализация Z-score]
    H --> I[Инференс модели]
    I --> J{Вероятность > Порог?}
    J -- Да --> K[Блокировка IP источника]
    K --> L[Запись инцидента в БД]
    J -- Нет --> M[Обновление статистики]
    L --> N{Сигнал Stop?}
    M --> N
    N -- Нет --> F
    N -- Да --> O[Закрытие ресурсов]
    O --> P([Конец])
```

---

### Лист 3. Схема алгоритма извлечения признаков

Отражает логику работы модуля `FeatureExtractor`.

```mermaid
flowchart TD
    A([Вход: RawPacket]) --> B[Парсинг Ethernet]
    B --> C{Протокол?}
    C -- IPv4 --> D[Извлечение IP src/dst, TTL]
    C -- Прочее --> E[Пропуск]
    D --> F{L4 Протокол?}
    F -- TCP --> G[Извлечение флагов SYN/ACK/RST, портов]
    F -- UDP --> H[Извлечение портов]
    F -- ICMP --> I[Извлечение типа/кода]
    G --> J[Обновление окна статистики источника]
    H --> J
    I --> J
    J --> K[Расчет PPS, ByteRate, SynRatio]
    K --> L[Применение логарифмирования log1p]
    L --> M[Стандартизация Z-score]
    M --> N([Выход: FeatureVector])
```

---

### Лист 4. Схема структуры базы данных (ER-диаграмма)

Описывает таблицы PostgreSQL и их связи.

```mermaid
erDiagram
    detection_events {
        bigint id PK
        timestamptz timestamp
        inet src_ip
        inet dst_ip
        smallint protocol
        varchar attack_type
        double probability
        varchar model_name
    }
    blocked_ips {
        inet ip_address PK
        timestamptz blocked_at
        timestamptz expires_at
        text reason
    }
    statistics {
        bigint id PK
        timestamptz timestamp
        integer total_packets
        integer attack_packets
        bigint byte_count
    }

    detection_events }|--|| blocked_ips : "связь по src_ip"
```

---

### Лист 5. Скриншоты графического интерфейса

Рекомендуемая компоновка для плаката/слайда:
1.  **Центральная часть**: Главное окно `ddos_monitor` с графиком PPS в момент атаки (красная зона).
2.  **Левая часть**: Таблица `Event History` с заполненными данными об атаках (SYN Flood, UDP Flood).
3.  **Правая часть**: Панель настроек с выбором модели (XGBoost / MLP / RF) и статус подключения к коллектору.
4.  **Нижняя часть**: Виджет системного лога с предупреждениями о блокировке IP.

*Примечание: Скриншоты следует делать в темной теме оформления для лучшей визуальной эффектности.*
