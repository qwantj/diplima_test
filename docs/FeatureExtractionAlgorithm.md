# Алгоритм извлечения признаков

**Версия:** 2.2  
**Дата актуализации:** 21.05.2026  
**Реализация:** [`src/network/FeatureExtractor.cpp`](../src/network/FeatureExtractor.cpp)

---

## 1. Контекст и назначение

`FeatureExtractor` — модуль, преобразующий поток сырых сетевых пакетов в числовой вектор признаков для ML-модели.

**Принцип работы:** каждые 2 секунды (`INFERENCE_WINDOW_SEC`) собирается окно пакетов. По завершении окна вычисляется набор статистических признаков на основе потоко-ориентированной агрегации (flow-based statistics), которые затем нормализуются и передаются в `ModelInferencer::predict()`.

---

## 2. Структуры данных

### FlowKey — ключ потока

Двунаправленный 5-tuple ключ (IP минимальный/максимальный, порт, протокол):

```cpp
struct FlowKey {
    std::string ip1, ip2;       // Упорядоченная пара (min, max)
    uint16_t    port1, port2;
    uint8_t     proto;          // 6=TCP, 17=UDP, 1=ICMP
};
```

### FlowStats — статистика потока

```cpp
struct FlowStats {
    std::string initiatorIp;                      // IP инициатора (SYN)
    std::chrono::steady_clock::time_point firstPacketTime;
    std::chrono::steady_clock::time_point lastPacketTime;

    uint64_t fwdPackets = 0, bwdPackets = 0;
    uint64_t fwdBytes   = 0, bwdBytes   = 0;

    // TCP флаги
    uint64_t synPackets = 0, ackPackets = 0;
    uint64_t finPackets = 0, rstPackets = 0;
    uint64_t pshPackets = 0, urgPackets = 0;
    uint64_t tcpWindowSizeTotal = 0;

    // Для энтропии
    std::vector<uint64_t> byteCounts;             // 256 элементов
    uint64_t totalPayloadBytes = 0;
};
```

---

## 3. Шаги алгоритма

```
Входящие пакеты (из TrafficMonitor)
           │
           ▼
┌───────────────────────────────────────────┐
│  processPacket(PacketBuffer* pkt)          │
│                                           │
│  1. Разбор пакета (Ethernet→IP→TCP/UDP)   │
│  2. Определение FlowKey (5-tuple)          │
│  3. Обновление FlowStats:                 │
│     - fwdPackets/bwdPackets               │
│     - fwdBytes/bwdBytes                   │
│     - TCP flags (SYN, ACK, FIN, RST...)   │
│  4. Обновление глобальных счётчиков:      │
│     - totalPackets_, tcpPackets_...        │
│     - srcIpCounts_, dstPortCounts_        │
└───────────────────────────────────────────┘
           │ (по истечении 2 сек)
           ▼
┌───────────────────────────────────────────┐
│  computeNormalizedFeatures()              │
│                                           │
│  1. Найти наиболее нагруженный поток      │
│  2. Вычислить 8 признаков                │
│  3. Применить нормализацию (scaler)       │
│  4. Вернуть вектор float[8]              │
└───────────────────────────────────────────┘
           │
           ▼
    ModelInferencer::predict()
```

---

## 4. Извлекаемые признаки (8)

Порядок и состав признаков определяются файлом `rf_scaler_params.json` / `mlp_scaler_params.json` (поле `"features"`).

| № | Название | Формула | Единица |
|---|---|---|---|
| 1 | **Flow Duration** | `lastPacketTime - firstPacketTime` | мкс |
| 2 | **Total Fwd Packets** | `fwdPackets` | шт |
| 3 | **Total Backward Packets** | `bwdPackets` | шт |
| 4 | **Total Length of Fwd Packets** | `fwdBytes` | байт |
| 5 | **Total Length of Bwd Packets** | `bwdBytes` | байт |
| 6 | **Fwd Packet Length Mean** | `fwdBytes / max(fwdPackets, 1)` | байт |
| 7 | **Bwd Packet Length Mean** | `bwdBytes / max(bwdPackets, 1)` | байт |
| 8 | **Flow Packets/s** | `(fwdPackets + bwdPackets) / duration_sec` | пакет/с |

---

## 5. Нормализация (StandardScaler)

Для каждого признака `i` применяется формула:

```
X_norm[i] = (X[i] - mean[i]) / scale[i]
```

Параметры `mean` и `scale` берутся из соответствующего JSON-файла:
- `rf_model.onnx` → `rf_scaler_params.json`
- `mlp_model.onnx` → `mlp_scaler_params.json`

### Пример параметров (rf_scaler_params.json)

```json
{
  "features": ["Flow Duration", "Total Fwd Packets", ...],
  "mean":  [1190047.0, 4.22, 0.099, 1977.7, 11.8, 600.6, 0.356, 1111564.0],
  "scale": [6487791.0, 186.4, 3.068, 5272.6, 5552.8, 461.0, 10.8, 921722.0],
  "use_log1p": false
}
```

### Режим log1p (для MLP)

Если `"use_log1p": true`:

```
X[i] = log(1 + max(0, X[i]))   # перед нормализацией
X_norm[i] = (X[i] - mean[i]) / scale[i]
```

---

## 6. Автоматический выбор скейлера

`DetectionEngine::init()` и `hotSwapModel()` вызывают `FeatureExtractor::setModelScaler(modelPath)`:

```cpp
void FeatureExtractor::setModelScaler(const std::string& modelPath) {
    // rf_model.onnx → ищем rf_scaler_params.json
    // mlp_model.onnx → ищем mlp_scaler_params.json
    // Fallback → scaler_params.json
    auto scalerPath = deriveScalerPath(modelPath);
    loadScalerParams(scalerPath);
}
```

---

## 7. Дополнительная телеметрия окна

`fillTelemetry(DetectionResult& result)` заполняет поля, не используемые напрямую в ML, но необходимые для визуализации:

| Поле DetectionResult | Источник | Описание |
|---|---|---|
| `pps` | `totalPackets_ / elapsed_sec` | Пакетов в секунду |
| `tcpPackets`, `udpPackets`, `icmpPackets` | счётчики окна | Разбивка по протоколам |
| `synPackets`, `finPackets`, `rstPackets` | счётчики TCP | TCP-флаги |
| `totalBytes` | `totalBytes_` | Суммарный трафик, байт |
| `topTalkers` | топ-5 по `srcIpCounts_` | Источники по числу пакетов |
| `topPorts` | топ-5 по `dstPortCounts_` | Целевые порты |
| `topTargets` | топ-5 по `targetCounts_` | dst_ip:port |
| `uniqueSourceCount` | `uniqueSources_.size()` | Уникальные src IP |
| `activeFlowsCount` | `activeFlows_.size()` | Активных потоков |
| `packetSizeHistogram` | 5 бинов по размеру | Распределение длин пакетов |

**Бины гистограммы размеров пакетов:**
- Бин 0: < 64 байт
- Бин 1: 64–255 байт
- Бин 2: 256–511 байт
- Бин 3: 512–1023 байт
- Бин 4: ≥ 1024 байт

---

## 8. Пороговый фильтр шума

В `DetectionEngine::processWindow()` перед инференсом проверяется:

```cpp
if (result.pps < NOISE_THRESHOLD_PPS) {
    result.label = 0;
    result.confidence = 0.0f;
    return; // не вызываем predict()
}
```

`NOISE_THRESHOLD_PPS = 50.0` пакет/с — защита от ложных срабатываний при низкоинтенсивном трафике.

---

## 9. Ограничения и рекомендации по улучшению

### Текущие ограничения

| Ограничение | Описание |
|---|---|
| Агрегация по окну | Признаки вычисляются для наиболее нагруженного потока в окне, а не stateful-tracking всех сессий |
| Без TCP reassembly | SYN/FIN/RST считаются, но TCP-сессии не отслеживаются полностью |
| Дрейф распределений | ML-модели обучены на CICIDS2017, реальный трафик может отличаться |
| Только Windows | PcapPlusPlus и FirewallManager ориентированы на Windows/Npcap |

### Рекомендации по улучшению

1. **Stateful flow tracking** — вести полноценные TCP-сессии с управлением состоянием (ESTABLISHED, CLOSE_WAIT...).
2. **Entropy features** — добавить энтропию payload как дополнительный признак (инфраструктура уже подготовлена: `byteCounts` в FlowStats).
3. **Drift detection** — логировать распределения признаков для мониторинга дрейфа (Population Stability Index).
4. **Online learning** — периодически дообучать модели на live-данных с подтверждённой разметкой.
5. **Feature importance** — использовать Feature Importance RF для отбора наиболее информативных признаков.
6. **Confidence calibration** — применить Platt scaling или isotonic regression для калибровки вероятностей MLP.

---

## 10. Диагностика качества детекции

### Признаки плохой работы модели

- Постоянные ложные срабатывания при нормальном трафике → проверить соответствие scaler модели.
- Пропуск атак (confidence < 0.5) → обновить модель на более репрезентативных данных.
- Аномально высокие значения признаков → включить `use_log1p: true` в scaler.

### Логирование признаков для анализа

```cpp
// Временный код для отладки в processWindow():
auto features = extractor_.computeNormalizedFeatures();
for (size_t i = 0; i < features.size(); ++i) {
    AppLogger::get()->debug("Feature[{}] = {:.4f}", i, features[i]);
}
```