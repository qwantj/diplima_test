# 📚 Материалы для изучения к дипломной работе

Здесь собран список документации, книг и статей, которые помогут разобраться в технологиях проекта: **C++, Qt 6, PcapPlusPlus, Machine Learning & ONNX**.

---

## 1. C++ и Qt 6

### 📘 Документация (Must Read)
*   **[Документация Qt 6](https://doc.qt.io/qt-6/)** — основной источник.
    *   [Qt Widgets](https://doc.qt.io/qt-6/qtwidgets-index.html) — для создания интерфейса.
    *   [Qt Charts](https://doc.qt.io/qt-6/qtcharts-index.html) — для графиков трафика.
    *   [Signals & Slots](https://doc.qt.io/qt-6/signalsandslots.html) — механизм связи объектов (критически важно понять!).

### 📕 Книги
*   **"C++17. The Complete Guide" (Nicolai Josuttis)** — если нужно подтянуть современный C++.
*   **"Qt 6 C++ GUI Programming Cookbook"** — рецепты для GUI.

### 🎥 Видео
*   [VoidRealms (YouTube)](https://www.youtube.com/user/VoidRealms) — отличные уроки по C++ и Qt.

---

## 2. Анализ сети и PcapPlusPlus

### 📘 Документация
*   **[PcapPlusPlus Documentation](https://pcapplusplus.github.io/docs/)** — очень понятная документация с примерами.
    *   [Reading/Writing PCAP files](https://pcapplusplus.github.io/docs/tutorials/pcap-files) — чтение файлов.
    *   [Packet Parsing](https://pcapplusplus.github.io/docs/tutorials/packet-parsing) — разбор заголовков (IP, TCP, UDP).
    *   [Live Traffic Capture](https://pcapplusplus.github.io/docs/tutorials/live-traffic-capture) — захват с интерфейса.

### 🧠 Теория сетей
*   **Модель OSI и TCP/IP**: освежить знания о заголовках пакетов (TCP Flags: SYN, ACK, FIN и т.д.), IP-адресации и портах.
*   **Wireshark**: научиться открывать PCAP файлы и смотреть внутрь пакетов вручную.

---

## 3. Машинное обучение (ML) и ONNX

### 📘 Документация
*   **[Scikit-learn User Guide](https://scikit-learn.org/stable/user_guide.html)** — всё про Random Forest, MLP и препроцессинг.
    *   [Random Forest](https://scikit-learn.org/stable/modules/ensemble.html#forests-of-randomized-trees)
    *   [Neural Network Models (Supervised)](https://scikit-learn.org/stable/modules/neural_networks_supervised.html)
*   **[ONNX Runtime C++ API](https://onnxruntime.ai/docs/api/c/)** — как запускать модели в C++.
    *   [Inference with C++ API](https://onnxruntime.ai/docs/tutorials/c_api.html) — пример кода.

### 🧠 Концепции
*   **Обучение с учителем (Supervised Learning)**: классификация.
*   **Метрики**: Accuracy, Precision, Recall, F1-Score (понадобятся для пояснительной записки).
*   **StandardScaler**: зачем нужна нормализация данных.

---

## 4. Базы данных (PostgreSQL)

*   **[PostgreSQL Tutorial](https://www.postgresqltutorial.com/)** — основы SQL.
*   **Qt SQL**: класс [QSqlDatabase](https://doc.qt.io/qt-6/qsqldatabase.html) — как подключаться из C++.

---

## 5. Датасет CIC-DDoS2019

*   **[Описание датасета](https://www.unb.ca/cic/datasets/ddos-2019.html)** — изучите, какие типы атак там есть (DrDoS_DNS, Syn, UDP-Lag и т.д.) и что означают признаки (Flow Duration, Fwd Packets и т.д.).

---

## 💡 Совет по изучению
Не нужно читать всё от корки до корки.
1.  **Начните с PcapPlusPlus**: запустите пример чтения PCAP файла.
2.  **Разберитесь с ONNX Runtime**: попробуйте загрузить модель и подать ей фиктивные данные.
3.  **Qt**: начните с простого окна с одной кнопкой и графиком.
