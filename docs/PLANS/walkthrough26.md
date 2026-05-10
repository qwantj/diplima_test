# Walkthrough: Восстановление GUI DDoS Dashboard

## Что было сделано

### 1. Полная перезапись DashboardWidget.hpp

Файл был полностью рассинхронизирован с реализацией в `DashboardWidget.cpp`. Изменения:

#### Классы вспомогательных виджетов

render_diffs(file:///c:/Dev/CXX/diploma_test/src/monitor_ui/DashboardWidget.hpp)

- **`SloHealthGrid`** → **`AlertGridWidget`** — другое имя класса, другой API (`pushPoint(int, float)` → `addHealthPoint(bool)`), другое хранилище (`deque<int>` → `deque<bool>`)
- **`NetworkTopologyWidget`** — удалена вложенная `struct TargetNode`, изменена сигнатура `updateData` → `updateTopology(vector<pair<string,uint64_t>>&, double)`, хранилище `vector<pair<string,uint64_t>>`
- **`HeatmapWidget`** — изменена сигнатура `addSnapshot` → `addData(QDateTime, map)`, добавлены поля `minTime_`, `maxTime_`, изменена структура `HeatPoint`

#### Поля DashboardWidget (20+ переименований)

| Было в .hpp | Стало (как в .cpp) |
|---|---|
| `lblCollectorStatus_` | `lblCollector_` |
| `lblCurrentPps_` | `lblPps_` |
| `lblModelName_` | `lblModel_` |
| `lblTrafficStatus_` | `lblStatus_` |
| `chkAutoBlock_` | `bpfCheckbox_` |
| `lblCpuTitle_/lblRamTitle_/lblRatioTitle_` | `cpuTitle_/ramTitle_/trafficTitle_` |
| `ratioChart_/ratioPie_` | `trafficChart_/trafficPie_` |
| `attackUpper_/attackArea_` | `attackConfidenceUpper_/attackConfidenceArea_` |
| `sloGrid_` | `sloHealth_` |
| `topoWidget_` | `topologyWidget_` |
| `srcTable_/tgtTable_` | `tableSources_/tableTargets_` |
| `tabDashboard_` | `tabSystem_` |

#### Добавлены недостающие поля
`bwTimeAxis_`, `bwAxis_`, `sizeCatAxis_`, `sizeValAxis_`, `packetSizeSeries_`, `bandwidthChart_`, `bandwidthSeries_`

#### Методы
`setupDashboardTab()` → `setupDashboard()`, `setupAnalyticsTab()` → `setupAnalytics()`, добавлены `setupUI()`, `updateDonuts()`, `updateSystemMetrics()`

### 2. Исправление иконок Sidebar (monitor_main.cpp)

render_diffs(file:///c:/Dev/CXX/diploma_test/src/monitor_main.cpp)

По скриншотам:
- **Event Log (Live)** — оранжевый квадрат (`#f9e2af`, был красный `#f38ba8`)
- **Security Incidents** — синий кружок (`#89b4fa`, `isCircle=true`; был жёлтый квадрат)

## Верификация

- ✅ Проект собран MSVC компилятором: `ddos_monitor.exe` — exit code 0, без ошибок компиляции и линковки
