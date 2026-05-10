# Задачи: Восстановление GUI DDoS Dashboard

- `[x]` Пересинхронизация DashboardWidget.hpp с DashboardWidget.cpp
  - `[x]` Заменить SloHealthGrid → AlertGridWidget
  - `[x]` Обновить NetworkTopologyWidget (updateData → updateTopology, другие поля)
  - `[x]` Обновить HeatmapWidget (addSnapshot → addData, добавить minTime_/maxTime_)
  - `[x]` Обновить все поля DashboardWidget (20+ переименований)
  - `[x]` Обновить методы DashboardWidget (setupUI, setupDashboard, setupAnalytics, updateDonuts, updateSystemMetrics)
- `[x]` Исправить иконки sidebar в monitor_main.cpp (Event Log → оранжевый, Security Incidents → синий кружок)
- `[x]` Сборка проекта и проверка — ddos_monitor.exe собран успешно
