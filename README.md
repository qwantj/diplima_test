# DDoS Detection System

Программный комплекс обнаружения атак типа «отказ в обслуживании» с использованием машинного обучения.

Разработано в рамках выпускной квалификационной работы (ВКР).
Студент: Дерюга А.А., группа А-08-22.

## Основные возможности

- **Захват трафика:** Реальное время (Npcap) и анализ дампов (PCAP).
- **ML-анализ:** Классификация трафика с помощью моделей Random Forest и MLP (ONNX Runtime).
- **База данных:** Хранение истории атак и сессий в PostgreSQL.
- **Визуализация:** Современный Dashboard на Qt 6 с графиками PPS, тепловыми картами и топологией сети.
- **Производительность:** Обработка до 130k+ пакетов в секунду (PPS) на стандартном железе.

## Быстрый старт (Windows + MSVC)

### Предварительные требования
1. **Visual Studio 2022** с установленным компонентом "Разработка классических приложений на C++".
2. **Qt 6.6+** (установлен через Qt Online Installer, компонент MSVC 2022 64-bit).
3. **vcpkg** для управления зависимостями.
4. **PostgreSQL 14+** (запущенный локально).

### Установка зависимостей
```powershell
vcpkg install pcapplusplus:x64-windows
vcpkg install onnxruntime:x64-windows
```

### Сборка
1. Откройте папку проекта в **VS Code**.
2. Убедитесь, что расширение **CMake Tools** активно.
3. Выберите пресет `Visual Studio Community 2022 Release - x64`.
4. Нажмите **Build**.

Для подробных инструкций см. [docs/windows-setup.md](docs/windows-setup.md).

CMake enables `ENABLE_PORTABLE_RUNTIME` to add `-static-libgcc -static-libstdc++` for MinGW builds.
If you turn it off, ensure `C:/msys64/mingw64/bin` is on PATH when launching `dos_detector.exe`.

## Настройка доступа к БД через переменные окружения

Вы можете передать учетные данные для подключения к базе данных через переменные окружения, что является более безопасным подходом, чем хранение их в файле `config.json`.
Приоритет конфигурации следующий:
1. Переменные окружения (`DDOS_DB_USER` и `DDOS_DB_PASS`).
2. Настройки в файле `config.json`.
3. Пустая строка (если параметры не заданы).

Пример использования в командной строке:
```bash
export DDOS_DB_USER=postgres
export DDOS_DB_PASS=my_secure_password
./ddos_collector
```
## Структура проекта

- `src/collector_main.cpp` — Программа сбора и анализа данных (Collector).
- `src/monitor_main.cpp` — Программа мониторинга и управления (Monitor).
- `src/common/DatabaseManager.cpp` — Взаимодействие с PostgreSQL.
- `models/` — Обученные ONNX модели и параметры скейлера.
- `docs/` — Полная техническая документация и графические материалы.

## Документация

- [Техническая спецификация](docs/DOCUMENTATION.md)
- [Графические материалы (диаграммы ВКР)](docs/diagrams.md)
- [Алгоритм извлечения признаков](docs/FeatureExtractionAlgorithm.md)
- [Отчет о соответствии заданию ВКР](docs/VKR_COMPLIANCE.md)

## Лицензия

Проект разработан исключительно в образовательных целях.
