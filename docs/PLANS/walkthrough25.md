# Восстановление кодовой базы DiplomDDoS — Walkthrough

## Что было сделано

Полное восстановление **~30 исходных файлов** проекта DDoS Detection System, которые были утрачены с диска и никогда не были закоммичены в git.

## Источники для восстановления

1. **Документация Obsidian** (`docs/`) — Codebase.md, DOCUMENTATION.md, FeatureExtractionAlgorithm.md, QUICK_START.md, TROUBLESHOOTING.md
2. **MOC-файлы** из `build/ddos_core_autogen/` и `build/ddos_monitor_autogen/` — восстановлены сигнатуры Q_OBJECT классов
3. **CMakeCache.txt** — зависимости, пути, параметры
4. **Файлы моделей** — `models/rf_model.onnx`, `models/mlp_model.onnx`, scaler JSONы
5. **Логи и результаты** — `ddos_monitor.log`, `scripts/results.txt`

## Созданные файлы

### CMake
- [CMakeLists.txt](file:///c:/Dev/CXX/diploma_test/CMakeLists.txt) — 3 targets: ddos_core (static lib), ddos_collector, ddos_monitor

### Common (инфраструктура)
| Файл | Назначение |
|------|------------|
| [AppLogger](file:///c:/Dev/CXX/diploma_test/src/common/AppLogger.hpp) | spdlog wrapper (console + file) |
| [Protocol](file:///c:/Dev/CXX/diploma_test/src/common/Protocol.hpp) | JSON serialization, DetectionResult, SessionInfo |
| [DatabaseManager](file:///c:/Dev/CXX/diploma_test/src/common/DatabaseManager.hpp) | PostgreSQL async batch writer, 4 таблицы |
| [TcpServer](file:///c:/Dev/CXX/diploma_test/src/common/TcpServer.hpp) | Broadcast TCP server с буфером |
| [TcpClient](file:///c:/Dev/CXX/diploma_test/src/common/TcpClient.hpp) | Auto-reconnect клиент |
| [DataBridge](file:///c:/Dev/CXX/diploma_test/src/common/DataBridge.hpp) | Фасад для monitor (TCP + DB) |
| [FileBuffer](file:///c:/Dev/CXX/diploma_test/src/common/FileBuffer.hpp) | Буфер с опциональным file backing |
| [SystemMetricsCollector](file:///c:/Dev/CXX/diploma_test/src/common/SystemMetricsCollector.hpp) | CPU/RAM через Windows API |

### Network (захват трафика)
| Файл | Назначение |
|------|------------|
| [TrafficMonitor](file:///c:/Dev/CXX/diploma_test/src/network/TrafficMonitor.hpp) | PcapPlusPlus live capture + pcap replay |
| [FeatureExtractor](file:///c:/Dev/CXX/diploma_test/src/network/FeatureExtractor.hpp) | 8 CICIDS features + normalization + telemetry |
| [PcapDumper](file:///c:/Dev/CXX/diploma_test/src/network/PcapDumper.hpp) | Labeled pcap dump writer |

### ML
| Файл | Назначение |
|------|------------|
| [ModelInferencer](file:///c:/Dev/CXX/diploma_test/src/ml/ModelInferencer.hpp) | ONNX Runtime C++ API, hot-swap |

### Core
| Файл | Назначение |
|------|------------|
| [DetectionEngine](file:///c:/Dev/CXX/diploma_test/src/core/DetectionEngine.hpp) | 2-sec inference loop, callbacks |

### Monitor UI (Qt6 GUI)
| Файл | Назначение |
|------|------------|
| [ThemePalette](file:///c:/Dev/CXX/diploma_test/src/monitor_ui/ThemePalette.hpp) | Catppuccin System/Dark/Light |
| [DashboardWidget](file:///c:/Dev/CXX/diploma_test/src/monitor_ui/DashboardWidget.hpp) | PPS chart, protocol pie, heatmap, alert grid |
| [LogWidget](file:///c:/Dev/CXX/diploma_test/src/monitor_ui/LogWidget.hpp) | Batch event log table |
| [SessionWidget](file:///c:/Dev/CXX/diploma_test/src/monitor_ui/SessionWidget.hpp) | DB session browser |
| [EventHistoryWidget](file:///c:/Dev/CXX/diploma_test/src/monitor_ui/EventHistoryWidget.hpp) | Timeline, filtering, CSV export |

### Entry points
| Файл | Назначение |
|------|------------|
| [collector_main.cpp](file:///c:/Dev/CXX/diploma_test/src/collector_main.cpp) | CLI: --interface, --pcap, --model, --db-* |
| [monitor_main.cpp](file:///c:/Dev/CXX/diploma_test/src/monitor_main.cpp) | MainWindow, sidebar, tray, toolbar |

## Исправления сборки

1. **PcapPlusPlus target** — vcpkg экспортирует `PcapPlusPlus::Pcap++`, а не `PcapPlusPlus::PcapPlusPlus`
2. **msys64/mingw64 конфликт** — Qt находил VulkanHeaders в `C:/msys64/mingw64/include`, вызывая ABI-конфликт MSVC/MinGW → добавлен `CMAKE_IGNORE_PATH`
3. **QStyle include** — `QApplication::style()` требует полного определения `QStyle`
4. **DataBridge дублирование** — `.cpp` был и в ddos_core и в ddos_monitor → убран из monitor

## Результат сборки

```
ddos_collector.exe ✅ (Release, MSVC x64)
ddos_monitor.exe   ✅ (Release, MSVC x64)
```

Директория: `build_restore/Release/`
