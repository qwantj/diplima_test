# Быстрый старт

  

Дата актуализации: 20.05.2026

## 1. Проверка окружения

```cmd
cmake --version
```

Убедитесь, что установлены:
- Visual Studio 2022 (MSVC)
- Qt 6.6+ (MSVC 2022 64-bit)
- PostgreSQL 14+
- Npcap
- vcpkg

## 2. Сборка

```cmd
cd c:\Dev\CXX\diploma_test
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

  

## 3. Подготовка базы данных

  

Создайте БД (если еще не создана):

  

```sql

CREATE DATABASE ddos_detection_db;

```

  

Таблицы создаются автоматически при первом подключении приложения.

  

## 4. Запуск коллектора

  

Убедитесь, что `config.json` содержит корректные параметры подключения к БД.

Откройте CMD/PowerShell от администратора:

  

```cmd

cd c:\Dev\CXX\diploma_test\build\Release

.\ddos_collector.exe --list-interfaces

.\ddos_collector.exe -i "WiFi"

```

  

Если интерфейс не находится, используйте точное имя из списка или Loopback.

  

## 5. Запуск монитора

  

Во втором окне:

  

```cmd

cd c:\Dev\CXX\diploma_test\build\Release

.\ddos_monitor.exe

```

  

По умолчанию monitor подключается к:

  

- collector: localhost:50050 (настраивается в `config.json`)

- PostgreSQL: localhost:5432

  

## 6. Тест атаки и активная защита

  

В третьем окне:

  

```cmd

python c:\Dev\CXX\diploma_test\scripts\ddos_test.py --duration 10 --size 1024

```

  

Проверьте в GUI:

  

- рост PPS и разделение трафика на сглаженных графиках протоколов;

- изменение label/confidence и срабатывание сетки алертов (SLO Grid);

- **Активная блокировка:** если включено, атакующий IP появится в списке заблокированных (модуль FirewallManager);

- запись в журнал событий с мягкой цветовой индикацией;

- всплывающее уведомление в системном трее ОС;

- перестроение тепловой карты портов и метрик ресурсов.

  

## 7. Офлайн-анализ дампа pcap

  

Вариант 1: через консоль collector

  

```cmd

.\ddos_collector.exe --pcap c:\path\capture.pcap

```

  

Вариант 2: через GUI

  

- кнопка Открыть PCAP;

- monitor отправляет load_pcap в collector;

- по завершении приходит replay_done.

  

## 8. Полезные функции и быстрые исправления

  

- **Горячая замена модели (hot swap):** используйте кнопку `📂` на панели инструментов для загрузки другой ONNX-модели. Сбор и мониторинг продолжатся без остановки процесса.

- **Экспорт в CSV:** используйте кнопку экспорта в журнале событий для сохранения результатов.

- Нет интерфейса: проверьте Npcap и права администратора.

- Нет подключения monitor: проверьте порт 50050 и `config.json`.

- Нет записи в БД: проверьте PostgreSQL и параметры в `config.json`.

  

Подробнее: TROUBLESHOOTING.md