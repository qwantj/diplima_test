# План развития проекта DDoS Detector

## 🏁 Этап Подготовки

- [x] Создать подробный план на основе Yandex Monium/Grafana
- [x] Получить одобрение от пользователя
- [x] Добавить windeployqt и принудительное применение компилятора MSVC в CMake

## 📌 Этап 0 — Дизайн-система (Monium/Grafana)

- [x] Определить палитру цветов (Dark, Light)
- [x] Создать классы ThemePalette (ThemePalette.hpp, ThemePalette.cpp)
- [x] Добавить логику определения системной темы (QSettings в Windows Registry)
- [x] Добавить переключатель тем в UI монитора (QToolBar + QComboBox)
- [x] Интегрировать ThemePalette в сборку CMake

## 🚀 Этап 1 — Оптимизация производительности (CPU & RAM)

- [x] Интегрировать Lock-Free очереди (`moodycamel::ConcurrentQueue`)
- [x] Заменить `std::mutex`-очередь в `TrafficMonitor`
- [ ] Оптимизировать Memory Pools (vector::reserve и Object Pool пакетов)
- [ ] Оптимизировать ML: настройки потоков ONNX Runtime и EP (CPU/DirectML)
- [ ] Переработать DatabaseManager под Batching (фоновая агрегация и вставка)

## 🛡 Этап 2 — Стабильность

- [ ] Внедрить систему защиты от переполнения памяти (Drop Rate / Backpressure)
- [ ] Перенести логирование на `spdlog` для устранения блокировок потоков
- [ ] Внедрить систему авто-установки BPF-фильтров

## 📊 Этап 3 — Инфографика (Monium Patterns)

- [ ] Реализовать Smart Tooltips (умные тултипы) для мульти-графиков
- [ ] Виджет: Матрица Здоровья (Alert Grid & SLO Panel)
- [ ] Виджет: Таймлайн Уптайма (Горизонтальная история инцидентов)
- [ ] Виджет: StackedAreaChart протоколов (TCP/UDP/ICMP)
- [ ] Переработать таблицу логов + Гистограмма плотности атак
- [ ] Топ Атакующих IP и таблица
- [ ] Карта топологии (Internet -> Node)
- [ ] Системные ресурсы монитора (Ring charts CPU/RAM)

## 💡 Этап 4 — Модели ONNX

- [ ] Загрузка модели через UI на лету (TCP DataBridge TCP_LOAD_MODEL)
- [ ] Hot Swap: безопасная подмена сессии ONNX
- [ ] Валидация входных тензоров при загрузке модели
