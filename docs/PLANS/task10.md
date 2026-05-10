# Сохранение PCAP дампов с маркировкой

- [x] Исследование архитектуры
- [x] План реализации (v2 — с учётом рисков OOM/IO/spam)
- [x] Реализация
  - [x] PcapDumper.hpp + PcapDumper.cpp
  - [x] Интеграция в DetectionEngine
  - [x] CLI-опция --pcap-dir в collector_main.cpp
  - [x] Обновить CMakeLists.txt
- [x] Сборка и проверка
  - [x] ddos_monitor ✅ (собран)
  - [x] ddos_collector ✅ (собран, исправлена ошибка `LinkLayerType` на Loopback-интерфейсах)
  - [x] Проверка дампов ✅ (в `benign.pcap` пишется нормальный трафик, в `attack.pcap` — атакующий)
