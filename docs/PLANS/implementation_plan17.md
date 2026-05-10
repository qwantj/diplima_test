# План реализации (Этап 3, Часть 1)

Этот план покрывает наиболее приоритетные задачи 3 этапа из `c:\Dev\CXX\diploma_test\implementation_plan.md`. Остальные визуализации будут добавлены в следующих итерациях.

## Цель (Goal Description)
Реализовать аналитику по Top Talkers (самые активные IP-адреса), Топ атакуемых портов, графику Bandwidth (пропускная способность в Мбит/с), отображению вероятности атаки и улучшить визуальную индикацию статуса (пульсирующая красная рамка при атаке).

## Proposed Changes

### 1. Collector & Core (Извлечение Данных)

#### [MODIFY] `src/network/FeatureExtractor.hpp`
- Добавление `std::unordered_map<std::string, int> m_ipCounts` и `std::unordered_map<uint16_t, int> m_portCounts`.
- Обновление структур для выдачи топ-5.

#### [MODIFY] `src/network/FeatureExtractor.cpp`
- Извлечение `srcIP` из `IPv4Layer` и `dstPort` из `TcpLayer`/`UdpLayer` в `processPacket`.
- Метод `getTopTalkers(int count = 5)` и `getTopPorts(int count = 5)`, которые будут сортировать мапы по завершении окна (1 сек) и очищать (reset).

#### [MODIFY] `src/core/DetectionEngine.hpp`
- Расширение структуры `DetectionResult`:
  ```cpp
  std::vector<std::pair<std::string, int>> topIps;
  std::vector<std::pair<uint16_t, int>> topPorts;
  float bandwidthMbps = 0.0f;
  ```

#### [MODIFY] `src/core/DetectionEngine.cpp`
- В `processStatsWorker()`: запрашивать у `FeatureExtractor` топы и добавлять в `DetectionResult`. Рассчитывать `bandwidthMbps = (totalFwdLength + totalBwdLength) * 8 / 1000000.0`.

### 2. Common (Сетевой Протокол)

#### [MODIFY] `src/common/Protocol.hpp` & `Protocol.cpp`
- В `serializeStats` / `deserializeStats`:
  - Добавить массив `top_ips`: `[{"ip": "1.2.3.4", "count": 100}, ...]`
  - Добавить массив `top_ports`: `[{"port": 80, "count": 50}, ...]`
  - Добавить `bw` (Bandwidth).

### 3. Monitor UI (Визуализация)

#### [MODIFY] `src/monitor_ui/DashboardWidget.hpp`
- Добавление `QTableWidget* m_topIpsTable;` (для Top Talkers - задача 3.2).
- Добавление графиков `QPieSeries* m_portsPieSeries;` и `QChart* m_portsChart;` (Топ портов - задача 3.9).
- Добавление `QLineSeries* m_bandwidthSeries;` и `QChart* m_bandwidthChart;` (Bandwidth - задача 3.5).
- Добавление `QLabel* m_attackProbabilityLabel;` (задача 3.6).
- Добавление `QPropertyAnimation* m_alertAnimation;` (для пульсации - задача 3.11).

#### [MODIFY] `src/monitor_ui/DashboardWidget.cpp`
- Инициализация таблиц и графиков в премиальном стиле Monium / Grafana, соблюдая дизайн-систему (прозрачность, скругления, шрифты).
- Обновление `updateRealtime(const DetectionResult& result, uint64_t totalPackets)`:
  - Отрисовка % вероятности в `m_attackProbabilityLabel`.
  - Заполнение таблицы `m_topIpsTable`.
  - Обновление секторов пай-чарта `m_portsPieSeries`.
  - Обновление графика Bandwidth.
  - Если `result.label == 1` и `confidence > 0.8` -> запуск пульсации, иначе остановка.

## Verification Plan

### Automated Tests
- Сборка: `cmake --build build_msvc --config Release` 
- Убедиться, что все зависимости (включая QtCharts) компонуются без ошибок.

### Manual Verification
- Сгенерировать PCAP-файл или использовать скрипт атаки (`python scripts/ddos_test.py`).
- Запустить Collector и Monitor (можно в режиме Replay).
- Ожидаемый результат: 
  1. Появление красивой таблицы "Top Talkers" в Dashboard, обновляющейся каждую секунду.
  2. Появление "Top Ports" (Pie Chart).
  3. График Bandwidth отображает Мбит/с.
  4. Процент атаки (Probability) меняется в реальном времени.
  5. При атаке окно красиво пульсирует красной рамкой.
