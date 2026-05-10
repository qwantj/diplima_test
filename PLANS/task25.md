# GUI Reconstruction Tasks

## 1. Toolbar & MainWindow
- [ ] Заголовок → "DDoS Dashboard"
- [ ] Toolbar: Открыть PCAP + папка + Модель ComboBox + ● Live + Theme справа
- [ ] Убрать statusbar, перенести статусы в overview

## 2. Sidebar
- [ ] Цветные квадраты/круги вместо emoji
- [ ] Правильные подписи: Dashboard, Deep Analytics, Event Log (Live), Security Incidents, Sessions History
- [ ] Сузить до ~130px

## 3. Sessions History (SessionWidget)
- [ ] QTreeWidget → QTableWidget
- [ ] Столбцы: ID, Start Time, Interface/PCAP, Model, Attacks, Total Packets
- [ ] Attacks >0 → красный

## 4. Event Log (LogWidget)
- [ ] Столбцы: Time, Flow, Label, Confidence, PPS, Model
- [ ] Label: Benign=зелёный, Attack=красный

## 5. Security Incidents (EventHistoryWidget)
- [ ] Проверить столбцы таблицы
- [ ] Русские подписи

## 6. Dashboard
- [ ] Overview status bar (Collector, Total Packets, PPS, Drop Rate, Sources, Auto-Block, Model, Probability, ✔ Status)
- [ ] PPS Area chart с Attack Confidence (правая ось Y)
- [ ] 3 Donut charts: CPU, RAM, Traffic Ratio

## 7. Deep Analytics
- [ ] SLO Health grid 4×8
- [ ] Network Topology (node-link)
- [ ] IP tables (Sources + Targets)
- [ ] Top Ports donut
- [ ] Bandwidth line chart
- [ ] Heatmap
- [ ] Packet Size bar chart
