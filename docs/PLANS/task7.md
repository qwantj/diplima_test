# План реализации DDoS-детектора

## Фаза 1: Подготовка
- [x] Проектирование архитектуры и согласование плана
- [x] Создание `sql/init.sql`
- [x] Обновление `CMakeLists.txt` (разделение на 3 таргета)

## Фаза 2: База данных (PostgreSQL)
- [x] Определение схемы в `sql/init.sql`
- [x] Реализация `DatabaseManager.hpp`/`.cpp` в `src/common`

## Фаза 3: Общие компоненты и IPC
- [x] Определение протокола `Protocol.hpp`
- [x] Реализация локального буфера `FileBuffer.hpp`/`.cpp`
- [x] Реализация TCP-сервера `TcpServer.hpp`/`.cpp`
- [x] Реализация TCP-клиента `TcpClient.hpp`/`.cpp`

## Фаза 4: Компонент `ddos_collector.exe`
- [x] Рефакторинг `DetectionEngine` (добавление колбэков, отвязка от UI)
- [x] Реализация `collector_main.cpp` (CLI парсер, интеграция компонентов)

## Фаза 5: Компонент `ddos_monitor.exe` (Qt GUI)
- [x] Создание/рефакторинг виджетов (`DashboardWidget`, `LogWidget`, `SessionWidget`)
- [x] Создание `DataBridge` для связи БД и TCP с UI
- [x] Рефакторинг `monitor_main.cpp` (интеграция Qt SQL, QCharts и TCP)

## Фаза 6: Тестирование и отладка
- [x] Проверка сборки (CMake + MSVC)
- [ ] Тестирование записи в базу и локальный буфер
- [ ] Тестирование TCP стрима (реалтайм PPS график)
- [ ] E2E тест с `ddos_test.py`
