
import re

with open('docs/academic/ВКР_черновик_claude.md', 'r', encoding='utf-8') as f:
    text = f.read()

# 1. Extract Diagram 1 (Structural)
diag1 = """```mermaid
flowchart TB
  subgraph Collector["ddos_collector (фоновый процесс)"]
    TM["TrafficMonitor\\n(PcapPlusPlus / Npcap)"]
    FE["FeatureExtractor\\n(агрегация потоков, z-score)"]
    MI["ModelInferencer\\n(ONNX Runtime)"]
    DE["DetectionEngine\\n(оркестрация)"]
    FM["FirewallManager\\n(netsh advfirewall)"]
    DB1["DatabaseManager\\n(libpqxx, async write)"]
    TCP1["TcpServer\\n(JSON-стриминг)"]
  end
  subgraph Monitor["ddos_monitor (Qt6 GUI)"]
    DW["DashboardWidget\\n(графики, статистика)"]
    EH["EventHistoryWidget\\n(история событий)"]
    RG["ReportGenerator\\n(CSV экспорт)"]
    DB2["DatabaseManager\\n(чтение)"]
    TCP2["TcpClient\\n(подключение)"]
  end
  subgraph External["Внешние компоненты"]
    PG[("PostgreSQL")]
    NET["Сеть / Npcap"]
    FW["Windows Firewall"]
  end

  NET --> TM --> DE
  DE --> FE --> MI --> DE
  DE --> FM --> FW
  DE --> DB1 --> PG
  DE --> TCP1 --> TCP2 --> Monitor
  DB2 --> PG
  DW & EH & RG --> DB2
```"""

# 2. Extract Diagram 2 (Main Cycle)
diag2 = """```mermaid
flowchart TD
  Start(["Старт"]) --> Init["Инициализация:\\nConfigManager, ModelInferencer,\\nDatabaseManager, TcpServer"]
  Init --> Capture["TrafficMonitor::startCapture()\\nзапуск захвата в lock-free очередь"]
  Capture --> LoopStart["Начало окна агрегации (1 с)\\nFeatureExtractor::reset()"]
  LoopStart --> Dequeue{"dequeuePacket()\\nпакет доступен?"}
  Dequeue -- Да --> Process["processPacket()\\nобновить FlowStats"]
  Process --> TimeCheck{"Окно\\n≥ 1 с?"}
  TimeCheck -- Нет --> Dequeue
  TimeCheck -- Да --> Telemetry["fillTelemetry()\\nвычислить PPS"]
  Dequeue -- Нет --> TimeCheck
  Telemetry --> Noise{"PPS\\n< 50?"}
  Noise -- Да --> Benign["label=0, confidence=0\\n(порог шума)"]
  Noise -- Нет --> Features["computeNormalizedFeatures()\\nz-score нормализация"]
  Features --> Infer["ModelInferencer::predict()\\nONNX Runtime инференс"]
  Infer --> Incident["updateIncidentState()\\nконечный автомат атака↔норма"]
  Incident --> Block{"Атака\\nи mitigation ON?"}
  Block -- Да --> Firewall["FirewallManager::blockIp()\\nnetsh advfirewall add rule"]
  Block -- Нет --> Callback
  Firewall --> Callback["resultCallback_():\\nDatabaseManager::enqueueEvent()\\nTcpServer::send()"]
  Benign --> Callback
  Callback --> Running{"running_?"}
  Running -- Да --> LoopStart
  Running -- Нет --> Stop(["Стоп\\nclearAllRules()"])
```"""

# 3. Extract Diagram 3 (Feature Extraction)
diag3 = """```mermaid
flowchart TD
  P(["PacketBuffer* из очереди"]) --> Parse["Распаковка PcapPlusPlus:\\nEthernet → IP → TCP/UDP/ICMP"]
  Parse --> Tuple["Извлечь 5-tuple:\\nsrc_ip, dst_ip, src_port, dst_port, proto"]
  Tuple --> FindFlow{"activeFlows_\\n[FlowKey] существует?"}
  FindFlow -- Нет --> Create["Создать FlowStats\\nустановить initiatorIp, firstPacketTime"]
  FindFlow -- Да --> Update
  Create --> Update["Обновить FlowStats:\\nfwdPackets++, fwdBytes += len"]
  Update --> Proto{"proto?"}
  Proto -- TCP --> TCP_Flags["synPackets, ackPackets,\\nfinPackets, rstPackets,\\npshPackets, urgPackets++\\ntcpWindowSizeTotal += window"]
  Proto -- UDP --> UDP_End["fwdBytes += udp_len"]
  Proto -- ICMP --> ICMP_End["icmpPackets++"]
  TCP_Flags --> Global
  UDP_End --> Global
  ICMP_End --> Global["Глобальные счётчики окна:\\ntotalPackets++, totalBytes++\\nsrcIpCounts_[src_ip]++"]
  Global --> End(["Следующий пакет"])

  subgraph Normalize["По окончании окна: computeNormalizedFeatures()"]
    NF1["Для каждого FlowStats:\\nvector<double> raw = buildFeatureVector()"]
    NF2{"useLog1p?"}
    NF3["x = log1p(x)"]
    NF4["x' = (x - μᵢ) / σᵢ"]
    NF5["return normalized vector"]
    NF1 --> NF2
    NF2 -- Да --> NF3 --> NF4
    NF2 -- Нет --> NF4
    NF4 --> NF5
  end
```"""

# 4. Extract Diagram 4 (DB Schema)
diag4 = """```mermaid
erDiagram
  sessions {
    SERIAL id PK
    TEXT interface_name
    TEXT model_name
    TIMESTAMP start_time
    TIMESTAMP end_time
    BIGINT total_packets
    BIGINT total_attacks
    BIGINT total_benign
  }
  events {
    SERIAL id PK
    INTEGER session_id FK
    TIMESTAMP timestamp
    INTEGER label
    DOUBLE_PRECISION confidence
    DOUBLE_PRECISION pps
    BIGINT total_packets
    JSONB features
    TEXT model_name
  }
  security_events {
    SERIAL id PK
    INTEGER session_id FK
    TIMESTAMP start_time
    REAL duration_sec
    TEXT attacker_ip
    REAL pps_max
    TEXT type_label
    REAL confidence
  }
  stats_snapshots {
    SERIAL id PK
    INTEGER session_id FK
    TIMESTAMP timestamp
    REAL packets_per_s
    BIGINT total_packets
    INTEGER current_label
  }
  sessions ||--o{ events : "session_id"
  sessions ||--o{ security_events : "session_id"
  sessions ||--o{ stats_snapshots : "session_id"
```"""

app_e_content = f"""## ПРИЛОЖЕНИЕ Е

Графические материалы

### 1. Структурная схема программного комплекса

{diag1}

<p align="center">Рис. Е.1. Взаимодействие компонентов системы</p>

### 2. Алгоритм главного цикла обработки трафика

{diag2}

<p align="center">Рис. Е.2. Блок-схема процесса непрерывного мониторинга</p>

### 3. Алгоритм извлечения и нормализации признаков

{diag3}

<p align="center">Рис. Е.3. Потоковая обработка сетевых пакетов</p>

### 4. Логическая схема базы данных

{diag4}

<p align="center">Рис. Е.4. ER-диаграмма хранилища инцидентов</p>

### 5. Интерфейс пользователя (ddos_monitor)

![Интерфейс Dashboard](images/DDoS_Dashboard_1.png)

<p align="center">Рис. Е.5. Основное окно мониторинга в реальном времени</p>

![Интерфейс Аналитики](images/DDoS_Dashboard_Analytics.png)

<p align="center">Рис. Е.6. Виджет детальной сетевой аналитики</p>
"""

# Replace the placeholder in the main document
placeholder_pattern = r'### Приложение Е\. Графические материалы.*?\)\n'
text = re.sub(placeholder_pattern, app_e_content, text, flags=re.DOTALL | re.IGNORECASE)

with open('docs/academic/ВКР_черновик_claude.md', 'w', encoding='utf-8') as f:
    f.write(text)

print("Appendix E has been generated and injected.")
