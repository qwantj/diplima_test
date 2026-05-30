# Описание API и Протокола обмена

**Версия протокола:** 2.2  
**Дата актуализации:** 21.05.2026

---

## 1. Транспортный уровень

### Формат фреймирования

Каждое сообщение передаётся с **4-байтовым заголовком длины** (big-endian `uint32_t`), за которым следует JSON-тело.

```
┌─────────────────────┬──────────────────────────────────────┐
│  Length (4 bytes)   │  JSON payload (Length bytes)         │
│  uint32_t BE        │  {"type":"...", "version":"2.2", ... │
└─────────────────────┴──────────────────────────────────────┘
```

Реализация: `Protocol::frame(payload)` / `Protocol::parseMessage(raw)`.

### Подключение

- **Порт по умолчанию:** 50050
- **Хост:** 127.0.0.1 (настраивается в `config.json` → `collector_host`)
- **Протокол:** TCP

### Базовая структура сообщения

```json
{
  "type": "<тип_сообщения>",
  "version": "2.2",
  "data": { ... }
}
```

---

## 2. Сообщения: Collector → Monitor

### 2.1 `stats` — Полная телеметрия окна

Отправляется **каждые 1 секунда** (по завершении inference-окна). Содержит полный результат ML-классификации и телеметрию.

```json
{
  "type": "stats",
  "version": "2.2",
  "data": {
    "timestamp": "2026-05-21T14:30:00.123",
    "session_id": 42,
    "model_name": "rf_model.onnx",

    "label": 1,
    "confidence": 0.97,

    "pps": 12500.0,
    "total_packets": 1500000,
    "total_bytes": 750000000,
    "dropped_packets": 0,
    "queue_size": 1234,
    "inference_latency_ms": 0.8,

    "tcp_packets": 10000,
    "udp_packets": 2000,
    "icmp_packets": 500,
    "other_packets": 0,
    "syn_packets": 8000,
    "fin_packets": 100,
    "rst_packets": 50,

    "flow_duration": 2.0,

    "features": [1.2, 0.5, 0.0, 1.8, 0.0, 600.5, 0.0, 12500.0],

    "top_src_ips": [
      {"ip": "192.168.1.100", "count": 11000},
      {"ip": "10.0.0.5", "count": 1500}
    ],
    "top_dst_ports": [
      {"port": 80, "count": 10000},
      {"port": 443, "count": 2000}
    ],
    "top_targets": [
      {"target": "172.16.0.1:80", "count": 10500}
    ],
    "unique_sources": 3,
    "active_flows": 5,

    "packet_size_histogram": [500, 2000, 8000, 1500, 500],

    "blocked_ips": ["192.168.1.100"]
  }
}
```

**Описание полей:**

| Поле | Тип | Описание |
|---|---|---|
| `label` | int | 0 = норма, 1 = атака |
| `confidence` | float | Вероятность класса [0.0, 1.0] |
| `pps` | float | Пакетов в секунду в текущем окне |
| `total_packets` | uint64 | Общее число пакетов за сессию |
| `dropped_packets` | uint64 | Пакеты, потерянные при переполнении очереди |
| `queue_size` | uint64 | Текущий размер очереди пакетов |
| `inference_latency_ms` | float | Время инференса ML-модели, мс |
| `features` | array[8] | Нормализованный вектор признаков (порядок по scaler JSON) |
| `top_src_ips` | array | Топ source IP по числу пакетов |
| `top_dst_ports` | array | Топ destination port по числу пакетов |
| `top_targets` | array | Топ целей (dst_ip:port) по числу пакетов |
| `unique_sources` | uint32 | Число уникальных source IP |
| `active_flows` | uint32 | Число активных потоков в окне |
| `packet_size_histogram` | array[5] | Гистограмма: [<64, 64-256, 256-512, 512-1024, >1024] байт |
| `blocked_ips` | array | IP-адреса, заблокированные FirewallManager |

---

### 2.2 `snapshot` — Лёгкий периодический снимок

Отправляется одновременно со `stats`. Используется для быстрого обновления индикаторов без полной обработки.

```json
{
  "type": "snapshot",
  "version": "2.2",
  "data": {
    "pps": 12500.0,
    "total_packets": 1500000,
    "current_label": 1
  }
}
```

---

### 2.3 `notify` — Уведомление о событии

Отправляется при изменении состояния системы.

```json
{
  "type": "notify",
  "version": "2.2",
  "data": {
    "event": "<имя_события>",
    "session_id": 42,
    "path": "C:/dumps/attack.pcap"
  }
}
```

**Типы событий (`event`):**

| Событие | Описание | Дополнительные поля |
|---|---|---|
| `replay_started` | Начато воспроизведение PCAP | `path` — путь к файлу |
| `replay_done` | Воспроизведение завершено | — |
| `live_resumed` | Возврат к live-трафику | `session_id` — ID live-сессии |

---

### 2.4 `buffer` — Батч исторических сообщений

Если монитор был отключён, `FileBuffer` сохраняет данные и при восстановлении соединения отправляет их батчем.

```json
{
  "type": "buffer",
  "version": "2.2",
  "data": [
    { "type": "stats", "data": { ... } },
    { "type": "stats", "data": { ... } }
  ]
}
```

---

## 3. Команды: Monitor → Collector

Все команды имеют тип `cmd`. Команды обрабатываются в `collector_main.cpp` в обработчике `TcpServer::commandReceived`.

### 3.1 `load_pcap` — Загрузка PCAP-файла

```json
{
  "type": "cmd",
  "data": {
    "command": "load_pcap",
    "path": "C:\\dumps\\attack.pcap"
  }
}
```

Коллектор создаёт новую сессию (с префиксом `pcap:`) и запускает replay.

### 3.2 `stop_replay` — Остановка воспроизведения

```json
{
  "type": "cmd",
  "data": {
    "command": "load_pcap",
    "action": "stop_replay"
  }
}
```

Коллектор останавливает replay и отправляет `notify: live_resumed`.

### 3.3 `load_model` — Горячая замена ML-модели

```json
{
  "type": "cmd",
  "data": {
    "command": "load_model",
    "path": "models/mlp_model.onnx"
  }
}
```

Коллектор вызывает `DetectionEngine::hotSwapModel()` — потокобезопасная замена без остановки.

### 3.4 `config_bpf` — Управление BPF-фильтром

```json
{
  "type": "cmd",
  "data": {
    "command": "config_bpf",
    "enable": true
  }
}
```

### 3.5 `config_dump` — Включение/выключение записи PCAP-дампов

```json
{
  "type": "cmd",
  "data": {
    "command": "config_dump",
    "enable": true
  }
}
```

При `enable: true` дампы сохраняются в папку `pcap_dumps/` в директории сборки.

### 3.6 `config_update` — Обновление конфигурации

```json
{
  "type": "cmd",
  "data": {
    "command": "config_update",
    "config": {
      "db_host": "new_host",
      "tcp_port": 50051
    }
  }
}
```

### 3.7 `stop` — Завершение работы коллектора

```json
{
  "type": "cmd",
  "data": {
    "command": "stop"
  }
}
```

---

## 4. Коды констант (Protocol namespace)

Определены в `src/common/Protocol.hpp`:

```cpp
namespace Protocol {
    const std::string PROTOCOL_VERSION = "2.2";

    // Типы сообщений
    constexpr const char* MSG_STATS    = "stats";
    constexpr const char* MSG_SNAPSHOT = "snapshot";
    constexpr const char* MSG_CMD      = "cmd";
    constexpr const char* MSG_NOTIFY   = "notify";
    constexpr const char* MSG_BUFFER   = "buffer";

    // Команды
    constexpr const char* CMD_LOAD_PCAP     = "load_pcap";
    constexpr const char* CMD_STOP_REPLAY   = "stop_replay";
    constexpr const char* CMD_LOAD_MODEL    = "load_model";
    constexpr const char* CMD_CONFIG_BPF    = "config_bpf";
    constexpr const char* CMD_CONFIG_DUMP   = "config_dump";
    constexpr const char* CMD_CONFIG_UPDATE = "config_update";
    constexpr const char* CMD_STOP          = "stop";
}
```

---

## 5. Структуры данных C++

### DetectionResult

Основная структура, содержащая результат одного inference-окна. Определена в `src/common/Protocol.hpp`.

```cpp
struct DetectionResult {
    QDateTime   timestamp;
    int         label = 0;              // 0=benign, 1=attack
    float       confidence = 0.0f;
    double      pps = 0.0;
    uint64_t    totalPackets = 0;
    uint64_t    tcpPackets, udpPackets, icmpPackets, otherPackets;
    uint64_t    synPackets, finPackets, rstPackets;
    uint64_t    totalBytes;
    double      flowDuration;
    uint64_t    droppedPackets;
    size_t      queueSize;
    double      inferenceLatencyMs;

    std::vector<double>  features;       // Нормализованный вектор 8 признаков
    
    std::map<std::string, double> protocolBreakdown;
    std::vector<int>              packetSizeHistogram; // 5 бинов
    std::vector<std::pair<std::string, uint64_t>> topTalkers;
    std::vector<std::pair<uint16_t, uint64_t>>    topPorts;
    std::vector<std::pair<std::string, uint64_t>> topTargets;
    uint32_t uniqueSourceCount;
    uint32_t activeFlowsCount;
    std::vector<std::string> blockedIps;

    int         sessionId;
    std::string modelName;
};
```

### SessionInfo

```cpp
struct SessionInfo {
    int       id;
    QString   interfaceName;
    QString   modelName;
    QDateTime startTime;
    QDateTime endTime;
    uint64_t  totalPackets;
    uint64_t  totalAttacks;
    uint64_t  totalBenign;
};
```

---

## 6. Примеры использования (curl/nc)

### Подключение к коллектору через netcat

```powershell
# Подключиться и получить поток данных (raw bytes, 4-byte length prefix)
ncat 127.0.0.1 50050
```

### Тестовый клиент на Python

```python
import socket
import struct
import json

def recv_message(sock):
    raw_len = sock.recv(4)
    if len(raw_len) < 4:
        return None
    length = struct.unpack('>I', raw_len)[0]
    data = b''
    while len(data) < length:
        chunk = sock.recv(length - len(data))
        data += chunk
    return json.loads(data.decode('utf-8'))

def send_command(sock, cmd, data=None):
    payload = json.dumps({"type": "cmd", "data": {"command": cmd, **(data or {})}}).encode('utf-8')
    sock.send(struct.pack('>I', len(payload)) + payload)

# Подключение и получение данных
with socket.socket() as s:
    s.connect(('127.0.0.1', 50050))
    while True:
        msg = recv_message(s)
        if msg and msg.get('type') == 'stats':
            print(f"Label: {msg['data']['label']}, PPS: {msg['data']['pps']}")
```