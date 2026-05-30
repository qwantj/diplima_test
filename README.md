# DDoS Detection System

> Программный комплекс обнаружения атак типа «отказ в обслуживании» с использованием машинного обучения.

Разработано в рамках выпускной квалификационной работы (ВКР).  
**Студент:** Дерюга А.А., группа А-08-22.  
**Версия:** 2.2

---

## Оглавление

- [Описание](#описание)
- [Основные возможности](#основные-возможности)
- [Архитектура](#архитектура)
- [Требования](#требования)
- [Быстрый старт](#быстрый-старт)
- [Конфигурация](#конфигурация)
- [Запуск](#запуск)
- [Структура проекта](#структура-проекта)
- [Документация](#документация)

---

## Описание

DDoS Detection System — двухкомпонентный программный комплекс для мониторинга сетевой активности и обнаружения аномалий типа «отказ в обслуживании» (DoS/DDoS) в режиме реального времени. 

Система состоит из двух исполняемых файлов:
- **`ddos_collector.exe`** — консольная служба захвата и анализа трафика.
- **`ddos_monitor.exe`** — графический интерфейс мониторинга и управления (Qt 6).

Оба компонента общаются между собой по TCP (порт 50050 по умолчанию) с использованием собственного JSON-протокола. Данные также сохраняются в базу данных PostgreSQL для последующего анализа.

---

## Основные возможности

| Возможность | Описание |
|---|---|
| 🔴 **Live захват трафика** | Захват пакетов в реальном времени через Npcap (PcapPlusPlus) |
| 📂 **Анализ PCAP-дампов** | Воспроизведение и анализ файлов `.pcap` / `.pcapng` |
| 🧠 **ML-классификация** | Random Forest и MLP через ONNX Runtime (до 130k+ PPS) |
| 🔄 **Горячая замена модели** | Переключение ML-моделей без перезапуска (hot swap) |
| 🛡️ **Активное противодействие** | FirewallManager блокирует IP атакующих через `netsh advfirewall` |
| 🗄️ **Персистентность** | Хранение истории сессий и инцидентов в PostgreSQL |
| 📊 **Современный Dashboard** | Qt 6 GUI с графиками PPS, тепловыми картами, топологией сети |
| 🔔 **Системный трей** | Фоновая индикация уровня угрозы с уведомлениями |
| 📤 **Экспорт данных** | Экспорт событий в CSV с защитой от инъекций |
| 🎨 **Темы оформления** | Переключение между Dark и Light темами |

---

## Архитектура

```
┌────────────────────────────────┐          ┌──────────────────────────────────┐
│         ddos_collector         │  TCP/JSON │         ddos_monitor             │
│                                │ ─────────►│                                  │
│  TrafficMonitor (Npcap)        │           │  DashboardWidget (PPS, Heatmap)  │
│       │ lock-free queue        │           │  LogWidget (Event Log)           │
│  FeatureExtractor (8 features) │           │  EventHistoryWidget (Incidents)  │
│       │                        │◄──────────│  SessionWidget (Sessions)        │
│  ModelInferencer (ONNX RT)     │  Commands │                                  │
│       │                        │           └──────────────────────────────────┘
│  DetectionEngine (orchestrator)│                           │
│       │                        │                           │ libpqxx
│  FirewallManager (blockip)     │                           ▼
│  DatabaseManager ─────────────────────────────────► PostgreSQL DB
└────────────────────────────────┘
```

### Слои системы

- **`network/`** — захват (TrafficMonitor, Npcap), извлечение признаков (FeatureExtractor).
- **`core/`** — оркестрация детекции (DetectionEngine), активная блокировка (FirewallManager).
- **`ml/`** — загрузка ONNX-модели и предсказание (ModelInferencer).
- **`common/`** — протокол обмена, TCP-сервер/клиент, база данных, конфигурация, логирование.
- **`monitor_ui/`** — Qt 6 GUI: Dashboard, EventHistory, LogWidget, SessionWidget.

---

## Требования

### Программное обеспечение

| Компонент | Версия | Обязательно |
|---|---|---|
| Visual Studio 2022 | 17.x | ✅ |
| Qt | 6.6+ (MSVC 2022 x64) | ✅ |
| vcpkg | актуальная | ✅ |
| Npcap | 1.75+ | ✅ |
| PostgreSQL | 14+ | ⚠️ (опционально) |
| ONNX Runtime | 1.18+ | ✅ |

### Зависимости vcpkg

```powershell
vcpkg install pcapplusplus:x64-windows
vcpkg install libpqxx:x64-windows
vcpkg install postgresql:x64-windows
```

### ONNX Runtime (ручная установка)

Скачайте `onnxruntime-win-x64-*.zip` с [GitHub Releases](https://github.com/microsoft/onnxruntime/releases) и распакуйте в `C:/Lib/onnxruntime/` (должны присутствовать папки `include/` и `lib/`).

---

## Быстрый старт

### 1. Клонирование и настройка vcpkg

```powershell
git clone <repo_url> C:\Dev\CXX\diploma_test
cd C:\Dev\CXX\diploma_test
```

### 2. Сборка (MSVC + CMake)

```powershell
# В Developer PowerShell for VS 2022:
cmake -S . -B build_msvc -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.6.0/msvc2019_64"

cmake --build build_msvc --config Release
```

### 3. Настройка базы данных

```sql
-- В psql создайте базу:
CREATE DATABASE ddos_detection_db;
-- Таблицы создаются автоматически при первом запуске коллектора
```

### 4. Запуск

```powershell
# Терминал 1: запустите коллектор (от имени администратора)
.\build_msvc\Release\ddos_collector.exe -i "WiFi"

# Терминал 2: запустите монитор
.\build_msvc\Release\ddos_monitor.exe
```

Подробные инструкции по установке — см. [docs/windows-setup.md](docs/windows-setup.md).

---

## Конфигурация

Файл `config.json` (копируется в директорию сборки автоматически):

```json
{
    "collector_host": "127.0.0.1",
    "tcp_port": 50050,
    "database": {
        "host": "localhost",
        "port": 5432,
        "name": "ddos_detection_db",
        "user": "postgres",
        "password": "your_password"
    },
    "ml": {
        "default_model": "models/rf_model.onnx",
        "default_ep": "cpu",
        "window_sec": 1.0
    },
    "network": {
        "default_interface": "",
        "max_queue_size": 500000
    }
}
```

### Переменные окружения (приоритет выше config.json)

```powershell
$env:DDOS_DB_USER = "postgres"
$env:DDOS_DB_PASS = "my_secure_password"
.\ddos_collector.exe -i "WiFi"
```

---

## Запуск

### ddos_collector — параметры командной строки

| Параметр | Описание | Пример |
|---|---|---|
| `--list-interfaces` | Вывести список сетевых интерфейсов | — |
| `-i`, `--interface <name>` | Захват с интерфейса | `-i "WiFi"` |
| `-f`, `--pcap <file>` | Воспроизведение PCAP-файла | `-f attack.pcap` |
| `-m`, `--model <path>` | Путь к ONNX-модели | `-m models/rf_model.onnx` |
| `-e`, `--ep <provider>` | Execution provider: `cpu`\|`cuda`\|`dml` | `-e cpu` |
| `--tcp-port <port>` | Порт TCP-сервера | `--tcp-port 50050` |
| `--db-host <host>` | Хост PostgreSQL | `--db-host localhost` |
| `--db-port <port>` | Порт PostgreSQL | `--db-port 5432` |
| `--db-name <name>` | Имя базы данных | `--db-name ddos_detection_db` |
| `--db-user <user>` | Пользователь БД | `--db-user postgres` |
| `--db-password <pass>` | Пароль БД | `--db-password secret` |
| `--pcap-dir <dir>` | Директория для записи PCAP-дампов | `--pcap-dir pcap_dumps` |

### ddos_monitor — управление через GUI

- **Открыть PCAP** — загрузить файл для анализа в режиме offline.
- **⏹ Live** — вернуться к live-трафику после replay.
- **⏺ Запись** — включить/выключить запись PCAP-дампов.
- **Model** — выбрать активную ML-модель из папки `models/`.
- **Theme** — переключить тему (Dark/Light).

---

## Структура проекта

```
diploma_test/
├── src/
│   ├── collector_main.cpp      # Точка входа коллектора
│   ├── monitor_main.cpp        # Точка входа монитора
│   ├── core/
│   │   ├── DetectionEngine.cpp/.hpp   # Оркестратор детекции
│   │   └── FirewallManager.cpp/.hpp   # Блокировка IP
│   ├── network/
│   │   ├── TrafficMonitor.cpp/.hpp    # Захват пакетов (Npcap)
│   │   ├── FeatureExtractor.cpp/.hpp  # Вычисление 8 признаков
│   │   └── PcapDumper.cpp/.hpp        # Запись PCAP-дампов
│   ├── ml/
│   │   └── ModelInferencer.cpp/.hpp   # ONNX Runtime инференс
│   ├── common/
│   │   ├── Protocol.cpp/.hpp          # Структуры данных и сериализация
│   │   ├── DatabaseManager.cpp/.hpp   # PostgreSQL (async batch writer)
│   │   ├── TcpServer.cpp/.hpp         # TCP-сервер (коллектор)
│   │   ├── TcpClient.cpp/.hpp         # TCP-клиент (монитор)
│   │   ├── DataBridge.cpp/.hpp        # Мост данных монитора
│   │   ├── ConfigManager.cpp/.hpp     # Управление конфигурацией
│   │   ├── AppLogger.cpp/.hpp         # Логирование (spdlog)
│   │   ├── FileBuffer.cpp/.hpp        # Персистентный буфер событий
│   │   ├── SystemMetricsCollector.cpp # CPU/RAM метрики
│   │   ├── NetStructs.hpp             # Сетевые структуры
│   │   ├── PacketBuffer.hpp           # Пул буферов пакетов
│   │   └── CSVUtils.hpp               # Утилиты экспорта CSV
│   └── monitor_ui/
│       ├── DashboardWidget.cpp/.hpp   # Главный дашборд
│       ├── EventHistoryWidget.cpp/.hpp # История инцидентов
│       ├── LogWidget.cpp/.hpp         # Журнал событий
│       ├── SessionWidget.cpp/.hpp     # История сессий
│       ├── ThemePalette.cpp/.hpp      # Система тем (Dark/Light)
│       └── ReportGenerator.cpp/.hpp  # Генерация отчётов
├── models/
│   ├── rf_model.onnx              # Random Forest модель
│   ├── mlp_model.onnx             # MLP (нейросеть) модель
│   ├── rf_scaler_params.json      # Параметры нормализации для RF
│   └── mlp_scaler_params.json     # Параметры нормализации для MLP
├── docs/                          # Полная техническая документация
├── scripts/                       # Python скрипты (обучение моделей)
├── tests/                         # Unit тесты
├── datasets/                      # Датасеты для обучения
├── config.json                    # Конфигурация по умолчанию
└── CMakeLists.txt                 # Сборочная система
```

---

## Документация

| Документ | Описание |
|---|---|
| [docs/DOCUMENTATION.md](docs/DOCUMENTATION.md) | Полная техническая спецификация |
| [docs/API_REFERENCE.md](docs/API_REFERENCE.md) | Протокол обмена Collector↔Monitor |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Детальное описание архитектуры |
| [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) | Руководство разработчика |
| [docs/USER_GUIDE.md](docs/USER_GUIDE.md) | Руководство пользователя |
| [docs/windows-setup.md](docs/windows-setup.md) | Установка и настройка (Windows) |
| [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) | Устранение неполадок |
| [docs/FeatureExtractionAlgorithm.md](docs/FeatureExtractionAlgorithm.md) | Алгоритм извлечения признаков |
| [docs/DATABASE_SCHEMA.md](docs/DATABASE_SCHEMA.md) | Схема базы данных PostgreSQL |
| [docs/TESTING.md](docs/TESTING.md) | Тестирование системы |
| [docs/diagrams.md](docs/diagrams.md) | Диаграммы и схемы (ВКР) |

---

## Лицензия

Проект разработан исключительно в образовательных целях (ВКР).
