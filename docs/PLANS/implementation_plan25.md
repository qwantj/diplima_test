# Восстановление GUI DDoS Dashboard по скриншотам

## Описание

Полная реконструкция GUI ddos_monitor по 5 скриншотам оригинального интерфейса и спецификации `UI_Reconstruction_Spec.md`. Текущая реализация была восстановлена приблизительно — теперь нужно привести её в точное соответствие с оригиналом.

## Скриншоты оригинала (5 разделов)

````carousel
![1. Sessions History — таблица с ID, Start Time, Interface/PCAP, Model, Attacks (красные >0), Total Packets](C:/Users/андрюша/.gemini/antigravity/brain/tempmediaStorage/media__1778352646942.png)
<!-- slide -->
![2. Security Incidents — Timeline + таблица инцидентов + фильтры (Дата, Тип, IP)](C:/Users/андрюша/.gemini/antigravity/brain/tempmediaStorage/media__1778352649238.png)
<!-- slide -->
![3. Dashboard — Status bar, PPS area chart (с Confidence), 3 donut-диаграммы (CPU, RAM, Traffic Ratio)](C:/Users/андрюша/.gemini/antigravity/brain/tempmediaStorage/media__1778352657246.png)
<!-- slide -->
![4. Deep Analytics — SLO Health grid, Network Topology, IP tables, Port donut, Bandwidth chart, Heatmap, Packet Size bar chart](C:/Users/андрюша/.gemini/antigravity/brain/tempmediaStorage/media__1778352660139.png)
<!-- slide -->
![5. Event Log (Live) — таблица: Time, Flow, Label (цветной), Confidence, PPS, Model](C:/Users/андрюша/.gemini/antigravity/brain/tempmediaStorage/media__1778352663612.png)
````

## Сравнение текущей реализации vs оригинал

### Toolbar (верхняя панель)
| Элемент | Оригинал | Текущее | Нужно |
|---------|----------|---------|-------|
| Заголовок окна | "DDoS Dashboard" (зелёная полоса) | "DDoS Monitor — Real-time Detection" | Переименовать, стилизовать |
| "Открыть PCAP" кнопка | Слева с иконкой папки | Есть, но справа | Переместить влево |
| Кнопка папки (иконка) | Жёлтая папка для выбора каталога | Нет | **Добавить** |
| Выпадающий "Модель:" | ComboBox с именами моделей | Кнопка "Load Model" | **Заменить** на ComboBox |
| Индикатор "● Live" | Зелёный кружок + "Live" | Нет | **Добавить** |
| "Theme: Dark" | Справа | Есть | Переместить вправо |

### Sidebar (боковая панель)
| Элемент | Оригинал | Текущее | Нужно |
|---------|----------|---------|-------|
| Цветные иконки | ■ Dashboard (цветные блоки), ■ Deep Analytics, ■ Event Log (Live), ● Security Incidents, ■ Sessions History | Emoji 📊📈📋🔒📁 | Заменить на цветные квадраты/круги |
| Ширина | ~120px (узкая) | 200px | Сузить |

### Dashboard (раздел 1)
| Элемент | Оригинал | Текущее | Нужно |
|---------|----------|---------|-------|
| Status bar | Collector: Connected (Live), Total Packets, PPS, Drop Rate, Sources, Auto-Block BPF, Model, Probability, ✔ Normal Traffic | Упрощённые карточки | **Полностью переделать** |
| Главный chart | Area chart с PPS + TCP/UDP/ICMP слоями + Attack Confidence % (оранжевый) | Есть приближённо | Доработать легенду, 2 оси Y |
| Нижние виджеты | 3 donut-диаграммы (CPU/RAM/Traffic Ratio) | Есть AlertGrid + text | **Заменить на Qt Charts donuts** |

### Deep Analytics (раздел 2)
| Элемент | Оригинал | Текущее | Нужно |
|---------|----------|---------|-------|
| SLO Health grid | 4×8 зелёных прямоугольников | AlertGridWidget | Доработать |
| Network Topology | Node-link граф (Monitor → targets) | Нет | **Добавить** |
| IP таблицы | Top Sources + Top Targets | Нет в отдельном виде | **Добавить** |
| Top Active Ports | Donut chart | Нет | **Добавить** |
| Network Bandwidth | Line chart Mbps | Нет | **Добавить** |
| Heatmap | Port Activity Heatmap | Есть HeatmapWidget | Доработать |
| Packet Size Distribution | Bar chart 5 корзин | Нет | **Добавить** |

### Event Log (раздел 3)
| Элемент | Оригинал | Текущее | Нужно |
|---------|----------|---------|-------|
| Фильтр | "All Events" dropdown | Есть ComboBox | OK |
| Столбцы | Time, Flow, Label, Confidence, PPS, Model | Time, Level, Message, Source IP | **Переделать столбцы** |
| Label цвет | Benign=зелёный, Attack=красный | Есть | Проверить |

### Security Incidents (раздел 4)
| Элемент | Оригинал | Текущее | Нужно |
|---------|----------|---------|-------|
| Кнопки | Обновить, Экспорт в CSV | Есть | OK |
| Фильтры | Дата, Тип, IP | Есть | Проверить |
| Timeline | Горизонтальная полоса из квадратов (24ч) | TimelineWidget | Проверить |
| Таблица | Время начала, Длительность, IP, Max PPS, Тип, Уверенность | Есть | Проверить столбцы |

### Sessions History (раздел 5)
| Элемент | Оригинал | Текущее | Нужно |
|---------|----------|---------|-------|
| Формат | Плоская таблица (QTableWidget) | QTreeWidget | **Заменить на QTableWidget** |
| Столбцы | ID, Start Time, Interface/PCAP, Model, Attacks, Total Packets | Есть дерево | Плоская таблица |
| Attacks цвет | >0 = красный | Нет | **Добавить** |

## Proposed Changes

### Toolbar & MainWindow
#### [MODIFY] [monitor_main.cpp](file:///c:/Dev/CXX/diploma_test/src/monitor_main.cpp)
- Заголовок → "DDoS Dashboard"
- Toolbar: "Открыть PCAP" + папка кнопка + "Модель:" ComboBox + "● Live" индикатор + "Theme:" справа
- Убрать Reset Zoom из toolbar (переместить внутрь Dashboard)
- Убрать statusbar (информация в overview строке Dashboard)

---

### Dashboard
#### [MODIFY] [DashboardWidget.hpp](file:///c:/Dev/CXX/diploma_test/src/monitor_ui/DashboardWidget.hpp)
#### [MODIFY] [DashboardWidget.cpp](file:///c:/Dev/CXX/diploma_test/src/monitor_ui/DashboardWidget.cpp)
- **Overview строка**: Collector status, Total Packets, PPS, Drop Rate, Sources, Auto-Block, Model, Probability, ✔ Normal/Attack
- **Главный chart**: Area chart с PPS/TCP/UDP/ICMP/Other + Attack Confidence % (правая ось Y)
- **Нижние donuts**: 3 QPieChart donuts (CPU, RAM, Traffic Ratio)

---

### Deep Analytics
#### [MODIFY] [DashboardWidget.hpp](file:///c:/Dev/CXX/diploma_test/src/monitor_ui/DashboardWidget.hpp)
#### [MODIFY] [DashboardWidget.cpp](file:///c:/Dev/CXX/diploma_test/src/monitor_ui/DashboardWidget.cpp)
- **SLO Health**: 4×8 grid (AlertGridWidget доработать)
- **Network Topology**: Custom QWidget с paintEvent (node-link граф)
- **Таблицы IP**: 2 QTableWidget (Sources + Targets)
- **Top Ports Donut**: QPieChart
- **Bandwidth chart**: QLineSeries (Mbps)
- **Heatmap**: Улучшить HeatmapWidget
- **Packet Size Bar**: QBarChart

---

### Event Log
#### [MODIFY] [LogWidget.hpp](file:///c:/Dev/CXX/diploma_test/src/monitor_ui/LogWidget.hpp)
#### [MODIFY] [LogWidget.cpp](file:///c:/Dev/CXX/diploma_test/src/monitor_ui/LogWidget.cpp)
- Столбцы: Time, Flow, Label, Confidence, PPS, Model
- Label цветной: Benign=зелёный, Attack=красный

---

### Sessions History
#### [MODIFY] [SessionWidget.hpp](file:///c:/Dev/CXX/diploma_test/src/monitor_ui/SessionWidget.hpp)
#### [MODIFY] [SessionWidget.cpp](file:///c:/Dev/CXX/diploma_test/src/monitor_ui/SessionWidget.cpp)
- Заменить QTreeWidget → QTableWidget
- Столбцы: ID, Start Time, Interface/PCAP, Model, Attacks, Total Packets
- Attacks >0 → красный цвет

---

### Security Incidents
#### [MODIFY] [EventHistoryWidget.hpp](file:///c:/Dev/CXX/diploma_test/src/monitor_ui/EventHistoryWidget.hpp)
#### [MODIFY] [EventHistoryWidget.cpp](file:///c:/Dev/CXX/diploma_test/src/monitor_ui/EventHistoryWidget.cpp)
- Проверить и доработать столбцы таблицы
- Русские подписи как на скрине

---

## Open Questions

> [!IMPORTANT]
> 1. **SessionInfo struct** — сейчас в Protocol.hpp есть поля `interface`, `modelName`, `attackCount`, `totalPackets`?  Нужно проверить и дополнить если нет.
> 2. **Deep Analytics** — Network Topology: данные о связях Monitor→Target берутся из `topTargets` в DetectionResult?
> 3. **Модели в ComboBox** — откуда брать список моделей? Сканировать папку `models/`?

## Verification Plan

### Automated Tests
- `cmake --build build_restore --config Release` — сборка без ошибок
- Запуск `ddos_monitor.exe` — UI рисуется без крашей

### Manual Verification
- Визуальное сравнение каждой вкладки со скриншотами
- Проверка работы с реальными данными (pcap replay + live)
