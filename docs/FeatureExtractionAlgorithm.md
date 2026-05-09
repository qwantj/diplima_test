# Алгоритм извлечения признаков

  

Дата актуализации: 08.04.2026

  

## 1. Контекст

  

FeatureExtractor формирует входной вектор признаков для ONNX-модели на основе оконной агрегации трафика.

  

Окно инференса: 2 секунды.

  

## 2. Шаги алгоритма

  

1. Получить RawPacket из TrafficMonitor.

2. Разобрать пакет и определить:

   - протокол (TCP/UDP/ICMP/other),

   - длину пакета,

   - source/destination IP,

   - destination port (для топов).

3. Обновить статистику текущего окна.

4. По завершении окна вычислить 8 признаков.

5. Применить нормализацию по scaler params.

6. Передать вектор в ModelInferencer::predict.

  

## 3. Извлекаемые признаки (8)

  

Порядок определяется scaler JSON (features array), fallback — стандартный порядок:

  

1. Flow Duration

2. Total Fwd Packets

3. Total Backward Packets

4. Total Length of Fwd Packets

5. Total Length of Bwd Packets

6. Fwd Packet Length Mean

7. Bwd Packet Length Mean

8. Flow Packets/s

  

## 4. Формулы

  

Обозначения:

  

- Nfwd — число прямых пакетов.

- Nbwd — число обратных пакетов.

- Lfwd — сумма длин прямых пакетов.

- Lbwd — сумма длин обратных пакетов.

- DeltaT — длительность окна в секундах.

  

Тогда:

  

- FwdMean = Lfwd / Nfwd (если Nfwd > 0).

- BwdMean = Lbwd / Nbwd (если Nbwd > 0).

- FlowPacketsPerSec = (Nfwd + Nbwd) / DeltaT.

  

## 5. Нормализация

  

При наличии scaler применяется:

  

Xnorm = (X - mean) / scale

  

Если в scaler включен use_log1p, предварительно:

  

X = log(1 + max(0, X))

  

## 6. Дополнительная телеметрия окна

  

Помимо признаков extractor отдает:

  

- protocol breakdown (tcp/udp/icmp/other),

- packet size histogram (5 бинов),

- top talkers (source IP),

- top destination ports,

- top targets (dst_ip:port),

- unique source count.

  

## 7. Ограничения текущей реализации

  

- Агрегация ведется по окну интерфейса, а не по полноценному 5-tuple flow tracking.

- Качество зависит от совпадения train-time и run-time профиля признаков.

- Для MLP особенно чувствительны выбросы и сдвиги распределений.

  

## 8. Рекомендации по улучшению

  

1. Ввести расширенный flow key и stateful tracking.

2. Логировать распределения признаков для drift-анализа.

3. Добавить калибровку confidence на валидационном наборе.

4. Регулярно пересобирать scaler/model на обновленных live-данных.