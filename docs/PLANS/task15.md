# Этапы 2.1 - 2.3

## 2.1 Load Shedding / Backpressure
- [x] Ввести константу `MAX_QUEUE_SIZE` (например, 500 000 пакетов)
- [x] При переполнении очереди — отбрасывать новые пакеты (`drop`)
- [x] Добавить счётчик `dropped_packets` в `DetectionEngine` / `TrafficMonitor`
- [x] Передавать `drop_rate` через `DataBridge` в монитор для отображения

## 2.2 Асинхронное логирование (spdlog)
- [x] Подключить `spdlog` через CMake `FetchContent`
- [x] Заменить `std::cout` / `qDebug()` на `spdlog::info()`, `spdlog::warn()`, `spdlog::error()` во всех файлах коллектора
- [x] Настроить асинхронный sink (запись в файл `ddos_collector.log`)
- [x] Добавить ротацию логов (по размеру или дате)

## 2.3 Динамическая BPF-фильтрация (Авто-блокировка)
- [x] Добавить метод `TrafficMonitor::applyBpfFilter(const std::string& filter)`
- [x] При обнаружении атаки — автоматически устанавливать BPF-фильтр для блокировки Top-IP
- [x] BPF GUI Toggle: добавить чекбокс/кнопку в DashboardWidget
- [x] Передавать статус BPF-фильтрации (вкл/выкл) в коллектор через `MSG_CONFIG_UPDATE`

## 2.6 Извлечение признаков (Feature Extraction)
- [x] Формализовать список извлекаемых признаков (сейчас 8 в `FeatureExtractor`)
- [x] Реализовать метод `FeatureExtractor::getFeatureDocumentation()` для генерации описания признаков
- [x] Описать алгоритм обработки заголовков (L2/L3/L4) и агрегации во временных окнах
- [x] Схема алгоритма (docs/FeatureExtractionAlgorithm.md)

## 2.7 Журнал событий и БД (EventLog)
- [x] Создать таблицу `security_events` в PostgreSQL
- [x] Добавить асинхронную запись устойчивых атак (>= 3 окон) в `DetectionEngine`
- [x] Создать виджет `EventHistoryWidget` для монитора (таблица, фильтры, дата)
- [x] Реализовать экспорт инцидентов в CSV

## 🚀 Этап 3 — Интеграция паттернов Monium и Инфографика (UI)
- [/] Стек-диаграмма протоколов (Traffic Breakdown) -> Изменено на Line Chart по просьбе пользователя, исправлено сглаживание (PPS rate)
- [ ] Таблица Top-5 IP (Top Talkers)
- [ ] Вероятность атаки (%) — крупная панель Stat
- [ ] Системные ресурсы (Donut CPU/RAM)
- [ ] Таймлайн состояния системы (Uptime лента)
