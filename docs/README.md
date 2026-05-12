# Система обнаружения DDoS-атак

  

Версия документации: 2.2  

Дата актуализации: 08.04.2026

  

## 1. О проекте

  

DDoS Detection System — программный комплекс на C++17 для:

  

- захвата сетевого трафика в режиме реального времени;

- анализа pcap-дампов в офлайн-режиме;

- извлечения статистических признаков потока;

- классификации трафика ML-моделью в формате ONNX;

- хранения событий и инцидентов в PostgreSQL;

- визуализации состояния системы в GUI на Qt 6.

  

Проект соответствует постановке ВКР: содержит программу сбора и анализа данных, программу мониторинга и управления, а также интеграцию с реляционной БД.

  

## 2. Компоненты комплекса

  

| Компонент | Тип | Назначение |

|---|---|---|

| ddos_core | Статическая библиотека | Общее ядро: захват, признаки, инференс, протокол, БД |

| ddos_collector | Консольный исполняемый файл | Сбор трафика, ML-классификация, запись в БД, TCP-сервер |

| ddos_monitor | Графический исполняемый файл | Дашборд, журнал, сессии, управление моделями и replay |

  

## 3. Ключевые возможности

  

- Захват трафика через PcapPlusPlus/Npcap.

- Офлайн-анализ pcap через аргумент --pcap и через команду load_pcap из GUI.

- Извлечение 8 признаков CICIDS-совместимого вектора.

- Классификация через ONNX Runtime с поддержкой **горячей замены моделей (hot swap)** без остановки системы.

- Асинхронная пакетная запись в PostgreSQL.

- Обмен Collector <-> Monitor по TCP (по умолчанию localhost:50050).

- **Мониторинг системных ресурсов:** прямая аналитика CPU, RAM и трекинга доли вредоносного трафика.

- **Продвинутая визуализация:** тепловые карты портов (Heatmap), сетка инцидентов (SLO Alert Grid), топологии и раздельные сглаженные графики по протоколам (TCP/UDP/ICMP).

- **Системный трей:** фоновое отслеживание статуса безопасности с наглядной индикацией.

- Настраиваемый макет (layout) виджетов через встроенный JSON-маппинг.

  

## 4. Технологический стек

  

- C++17

- Qt 6 (Core, Network, Widgets, Charts, Sql)

- PcapPlusPlus

- ONNX Runtime

- PostgreSQL

- spdlog

- nlohmann/json

- moodycamel::ConcurrentQueue

  

## 5. Требования

  

- Windows 10/11

- Visual Studio 2022 (MSVC)

- CMake 3.16+

- Qt 6.10.x MSVC kit

- Npcap (режим WinPcap-compatible)

- PostgreSQL 14+

- ONNX Runtime binaries в C:/Lib/onnxruntime

- vcpkg зависимости (PcapPlusPlus, libpq)

  

Важно: в CMakeLists зафиксирована сборка под MSVC. MinGW/GCC не поддерживаются.

  

## 6. Сборка

  

```cmd

cd c:\Dev\CXX\diploma_test

mkdir build

cd build

cmake -G "Visual Studio 17 2022" -A x64 ..

cmake --build . --config Release

```

  

Исполняемые файлы после сборки:

  

- build\Release\ddos_collector.exe

- build\Release\ddos_monitor.exe

  

## 7. Быстрый запуск

  

1. Запустите collector от имени администратора:

  

```cmd

cd c:\Dev\CXX\diploma_test\build\Release

.\ddos_collector.exe --list-interfaces

.\ddos_collector.exe -i "WiFi"

```

  

2. В отдельном окне запустите monitor:

  

```cmd

cd c:\Dev\CXX\diploma_test\build\Release

.\ddos_monitor.exe

```

  

3. Опционально сгенерируйте тестовую UDP-нагрузку:

  

```cmd

python c:\Dev\CXX\diploma_test\scripts\ddos_test.py --duration 10

```

  

## 8. Параметры командной строки ddos_collector

  

- -i, --interface <name>

- -f, --pcap <file>

- --list-interfaces

- -m, --model <path> (по умолчанию: models/rf_model.onnx)

- -e, --ep <cpu|cuda|dml> (по умолчанию: cpu)

- --tcp-port <port> (по умолчанию: 50050)

- --db-host <host> (по умолчанию: localhost)

- --db-port <port> (по умолчанию: 5432)

- --db-name <name> (по умолчанию: ddos_detection_db)

- --db-user <user> (по умолчанию: postgres)

- --db-password <pass> (по умолчанию: пустая строка, рекомендуется использовать переменную окружения DDOS_DB_PASS)

- --pcap-dir <dir> (опционально: сохранять размеченные дампы)

  

## 9. Структура документации

  

- QUICK_START.md — практический запуск за 5 минут

- TROUBLESHOOTING.md — типовые проблемы и решения

- DOCUMENTATION_INDEX.md — навигация по всем документам

- docs/DOCUMENTATION.md — техническая документация

- docs/Codebase.md — структура исходников

- docs/FeatureExtractionAlgorithm.md — алгоритм извлечения признаков

- docs/diagrams.md — графический материал и Mermaid-диаграммы

- PZ_SUMMARY_RU.md — единый сводный документ для ПЗ

  

## 10. Известные ограничения

  

- Текущая реализация признаков агрегирует окно трафика на уровне интерфейса, а не полный 5-tuple flow-tracking.

- Проект ориентирован на Windows/MSVC-окружение.

- Корректность классификации зависит от соответствия train-time и run-time распределений признаков.

  

## 11. Лицензии и сторонние компоненты

  

Сторонние лицензии из build и venv не являются проектной документацией и поставляются как часть внешних зависимостей.