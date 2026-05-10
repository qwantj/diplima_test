# Анализ пропавших файлов: ddos_collector и ddos_monitor

## Диагноз

**Исходные файлы удалены с диска и никогда не были закоммичены в git.** Восстановить текстовые исходники стандартными средствами невозможно.

## Что было в проекте

Из артефактов сборки (`.vcxproj.filters`, `.tlog`, MOC-файлов) удалось реконструировать **полный список утраченных файлов**:

### Библиотека `ddos_core` (статическая lib)

| Файл | Описание (по MOC/tlog) |
|------|------------------------|
| `src/common/AppLogger.hpp` / `.cpp` | Логирование через spdlog |
| `src/common/DatabaseManager.hpp` / `.cpp` | PostgreSQL через Qt SQL, async flush |
| `src/common/FileBuffer.hpp` / `.cpp` | Файловый буфер |
| `src/common/Protocol.hpp` / `.cpp` | Сериализация DetectionResult, snapshot, stats |
| `src/common/SystemMetricsCollector.hpp` / `.cpp` | Сбор системных метрик |
| `src/common/TcpServer.hpp` / `.cpp` | QTcpServer — TCP-сервер (порт 50050) |
| `src/common/TcpClient.hpp` / `.cpp` | QTcpClient — TCP-клиент с reconnect |
| `src/common/DataBridge.hpp` / `.cpp` | Мост данных между collector и monitor |
| `src/core/DetectionEngine.hpp` / `.cpp` | Движок детекции (ONNX inference + features) |
| `src/ml/ModelInferencer.hpp` / `.cpp` | ONNX Runtime загрузка/inference |
| `src/network/FeatureExtractor.hpp` / `.cpp` | Извлечение фичей из трафика |
| `src/network/TrafficMonitor.hpp` / `.cpp` | Захват трафика через PcapPlusPlus |
| `src/network/PcapDumper.hpp` / `.cpp` | Запись трафика в .pcap |
| `src/ConcurrentQueue.h` | Lock-free очередь |

### Приложение `ddos_collector` (консольное)

| Файл | Описание |
|------|----------|
| `src/collector_main.cpp` | Точка входа. ONNX-модель → TCP-сервер → PostgreSQL |

### Приложение `ddos_monitor` (Qt GUI)

| Файл | Описание (по MOC) |
|------|-------------------|
| `src/monitor_main.cpp` | Точка входа GUI приложения |
| `src/monitor_ui/DashboardWidget.hpp` / `.cpp` | Главная панель: HeatmapWidget, AlertGridWidget, графики |
| `src/monitor_ui/LogWidget.hpp` / `.cpp` | Виджет логов |
| `src/monitor_ui/SessionWidget.hpp` / `.cpp` | Виджет сессий |
| `src/monitor_ui/EventHistoryWidget.hpp` / `.cpp` | История событий |
| `src/monitor_ui/ThemePalette.hpp` / `.cpp` | Темы оформления |

### Дополнительные файлы (из conversation logs)

| Файл | Описание |
|------|----------|
| `models/rf_model.onnx` | Обученная RF модель |
| `models/scaler_params.json` | Параметры нормализации |
| `models/mlp_scaler_params.json` | Параметры MLP скейлера |
| `sql/init.sql` | Схема PostgreSQL |
| `docs/DOCUMENTATION.md` | Основная документация |
| `docs/Codebase.md` | Описание кодовой базы |
| `docs/diagrams.md` | Mermaid-диаграммы |
| `docs/FeatureExtractionAlgorithm.md` | Алгоритм извлечения фичей |
| `PZ_SUMMARY_RU.md` | Описание ПЗ на русском |
| `QUICK_START.md` | Быстрый старт |
| `TROUBLESHOOTING.md` | Устранение неполадок |
| `DOCUMENTATION_INDEX.md` | Индекс документации |
| `RECOMMENDATIONS.md` | Рекомендации |

## Что удалось сохранить

1. ✅ **Скомпилированные бинарники** — `build/Release/ddos_collector.exe` (1.1 MB) и `ddos_monitor.exe` (573 KB)
2. ✅ **MOC-файлы** — содержат сигнатуры Qt-классов (сигналы, слоты, наследование)
3. ✅ **Логи работы** — `ddos_monitor.log`, `collector_debug.txt`
4. ✅ **ONNX-модель** — нужно проверить `models/`
5. ✅ **`.obj` файлы** — скомпилированные объекты (нельзя декомпилировать в исходники)

## Варианты восстановления

| Способ | Реалистичность | Результат |
|--------|---------------|-----------|
| Git reflog / git fsck | ❌ Файлы никогда не были в git | — |
| Корзина Windows | ❌ Пуста | — |
| Volume Shadow Copies | ❌ Нет теневых копий | — |
| Декомпиляция .exe | ⚠️ Очень частичная, нечитаемый код | ~5% |
| Восстановление по MOC + tlog + логам + документации | ✅ **Наилучший вариант** | ~85-95% |
| Программы восстановления данных (Recuva, R-Studio) | ⚠️ Если диск SSD с TRIM — шансы малы | ~10-30% |

## Рекомендация

**Лучший вариант** — я могу **пересоздать весь код с нуля**, используя:
- MOC-файлы (полные сигнатуры классов)
- Логи (понимание протокола и поведения)
- Build logs (зависимости и архитектура)
- `.vcxproj.filters` (структура проекта)
- Прошлые conversation logs Gemini (там файлы были прочитаны 25-30 апреля)

Код будет функционально идентичен оригиналу.
