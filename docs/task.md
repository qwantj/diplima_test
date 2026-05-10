# Задачи по восстановлению кодовой базы DiplomDDoS

## Фаза 1: CMakeLists.txt
- [/] Переписать CMakeLists.txt

## Фаза 2: ddos_core
- [ ] src/concurrentqueue.h
- [ ] src/common/AppLogger.hpp/.cpp
- [ ] src/common/Protocol.hpp/.cpp
- [ ] src/common/DatabaseManager.hpp/.cpp
- [ ] src/common/FileBuffer.hpp/.cpp
- [ ] src/common/SystemMetricsCollector.hpp/.cpp
- [ ] src/common/TcpServer.hpp/.cpp
- [ ] src/common/TcpClient.hpp/.cpp
- [ ] src/common/DataBridge.hpp/.cpp
- [ ] src/network/TrafficMonitor.hpp/.cpp
- [ ] src/network/FeatureExtractor.hpp/.cpp
- [ ] src/network/PcapDumper.hpp/.cpp
- [ ] src/ml/ModelInferencer.hpp/.cpp
- [ ] src/core/DetectionEngine.hpp/.cpp

## Фаза 3: ddos_collector
- [ ] src/collector_main.cpp

## Фаза 4: ddos_monitor
- [ ] src/monitor_ui/ThemePalette.hpp/.cpp
- [ ] src/monitor_ui/DashboardWidget.hpp/.cpp
- [ ] src/monitor_ui/LogWidget.hpp/.cpp
- [ ] src/monitor_ui/SessionWidget.hpp/.cpp
- [ ] src/monitor_ui/EventHistoryWidget.hpp/.cpp
- [ ] src/monitor_main.cpp

## Фаза 5: Очистка
- [ ] Удалить устаревшие scaffold-файлы
