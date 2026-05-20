# Описание API и Протокола обмена (API Reference)

Комплекс использует легковесный протокол на базе JSON (JSON Lines) по TCP для связи между фоновым сервисом захвата трафика (`ddos_collector`) и графическим монитором (`ddos_monitor`).

Порт по умолчанию: **50050**.

## 1. Формат сообщений

Каждое сообщение — это один валидный JSON объект, завершающийся символом новой строки `\n`.

Базовая структура:
```json
{
  "type": "тип_сообщения",
  "version": "2.2",
  "data": { ... }
}
```

## 2. Сообщения от Collector к Monitor

### 2.1 `stats` (Полная статистика окна)
Отправляется каждые 2 секунды (длительность окна). Содержит результаты работы ML модели, полные топы и список заблокированных IP.

```json
{
  "type": "stats",
  "version": "2.2",
  "data": {
    "window_size_ms": 2000,
    "pps": 12500,
    "drop_rate": 0.0,
    "prediction": {
      "label": "Attack",
      "confidence": 0.98,
      "model": "mlp_model.onnx"
    },
    "protocols": {
      "TCP": 10000,
      "UDP": 2000,
      "ICMP": 500
    },
    "bpf_enabled": true,
    "top_src_ips": [{"ip": "192.168.1.100", "count": 12000}],
    "top_dst_ips": [{"ip": "10.0.0.5", "count": 12000}],
    "top_ports": [{"port": 80, "count": 10000}],
    "blocked_ips": ["192.168.1.100"]
  }
}
```

### 2.2 `snapshot` (Периодический снимок)
Легковесное сообщение для быстрого обновления статуса.

```json
{
  "type": "snapshot",
  "version": "2.2",
  "data": {
    "pps": 12500,
    "total_packets": 1500000,
    "current_label": 1
  }
}
```

### 2.3 `notify` (Уведомления)
Используется для отправки статусов (например, завершение чтения pcap).

```json
{
  "type": "notify",
  "version": "2.2",
  "data": {
    "message": "replay_done"
  }
}
```

### 2.4 `buffer` (Батчинг сообщений)
Если монитор был отключен, локальный буфер коллектора накапливает статистику и затем отправляет её одним массивом.

```json
{
  "type": "buffer",
  "version": "2.2",
  "data": [
    { "type": "stats", "data": {...} },
    { "type": "stats", "data": {...} }
  ]
}
```

## 3. Команды от Monitor к Collector (CMD)

Monitor может отправлять управляющие команды в Collector. Тип сообщения должен быть `cmd`.

### 3.1 Загрузка PCAP дампа
```json
{
  "type": "cmd",
  "data": {
    "command": "load_pcap",
    "filepath": "C:\\path\\to\\dump.pcap"
  }
}
```

### 3.2 Остановка воспроизведения
```json
{
  "type": "cmd",
  "data": {
    "command": "stop_replay"
  }
}
```

### 3.3 Смена ML модели
```json
{
  "type": "cmd",
  "data": {
    "command": "load_model",
    "filepath": "C:\\path\\to\\models\\rf_model.onnx"
  }
}
```

### 3.4 Включение/Выключение BPF фильтра (Auto-Block)
```json
{
  "type": "cmd",
  "data": {
    "command": "config_bpf",
    "enabled": true
  }
}
```

### 3.5 Обновление конфигурации
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

### 3.6 Завершение работы
```json
{
  "type": "cmd",
  "data": {
    "command": "stop"
  }
}
```