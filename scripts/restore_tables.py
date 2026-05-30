import re

def fix_all_table_glitches(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        text = f.read()

    # Match Table 1.1 Duplication
    # | ... |
    # ---
    # |
    # | ... |
    text = re.sub(r'\| ICMP Flood, Smurf \| .*? \|\n---\n\|\n\|', '| ICMP Flood, Smurf | Исчерпание пропускной способности каналов. Амплификация (DNS, NTP, SSDP) даёт коэффициент усиления до 4096 раз [3] |\n|', text)
    
    # Fix the weird | \n --- \n | pattern
    text = re.sub(r'\|\n---\n\|\n', '|\n', text)
    
    # Fix Section 1.5 duplication manually
    start_1_5 = text.find('<p align="right">Таблица 1.5</p>')
    if start_1_5 != -1:
        end_1_5 = text.find('### 1.6.', start_1_5)
        if end_1_5 != -1:
            t1_5_block = """<p align="right">Таблица 1.5</p>
<p align="center"><b>Средства разработки и обоснование выбора по требованиям NFR</b></p>

| Компонент | Технология | Версия | Закрываемое NFR | Обоснование выбора |
|---|---|---|---|---|
| Язык программирования | C++ | 17 | NFR-001, NFR-002 | Детерминированное управление памятью и производительность, критически важные для обработки >10 000 пак/с. C++17 предоставляет structured bindings, `std::filesystem`, `std::shared_mutex` — используются в коде |
| Захват пакетов | PcapPlusPlus / Npcap | 23.x / 1.79+ | NFR-001 | PcapPlusPlus — объектно-ориентированная обёртка над libpcap/Npcap. Npcap обеспечивает захват в режиме promiscuous на Windows |
| ML/DL-инференс | ONNX Runtime C++ API | 1.17+ | NFR-001, NFR-003 | Native C++ API не требует Python-интерпретатора в runtime; поддерживает XGBoost, scikit-learn, PyTorch; задержка инференса <1 мс; горячая замена модели (hot-swap) |
| Нормализация данных | Собственная реализация (z-score) | — | NFR-001 | Реализована в ~30 строк в методе `FeatureExtractor::computeNormalizedFeatures()`. Исключает зависимость от тяжёлых библиотек линейной алгебры |
| СУБД | PostgreSQL | 15+ | NFR-004, NFR-005 | Клиент-серверная РСУБД (RDBMS) con полной поддержкой ACID; пакетная обработка с использованием механизма потоковой записи (COPY) [20]; взаимодействие через библиотеку libpqxx |
| GUI | Qt | 6.x | NFR-002, NFR-006 | Богатый набор виджетов (Qt Charts, QSystemTrayIcon) для real-time отображения; кроссплатформенность; полная поддержка MSVC 2022 [21] |
| Конкурентные очереди | moodycamel ConcurrentQueue | 1.x | NFR-001 | Lock-free очередь (MPMC); обеспечивает потокобезопасный обмен данными между захватом и анализом |
| Логирование | spdlog | 1.x | NFR-007 | Библиотека логирования с поддержкой асинхронной записи и форматирования fmt |
| Сериализация | nlohmann/json | 3.x | — | Загрузка конфигурации и параметров масштабировщика |
| Сборка | CMake | 3.25+ | NFR-006 | Единая система сборки для Windows (MSVC) и Linux (GCC/Clang) |
| Компилятор | MSVC 2022 | — | NFR-006 | Полная поддержка C++17; оптимизации `/O2` |
| Управление пакетами | vcpkg | — | NFR-006 | Управление зависимостями C++ (libpqxx, nlohmann-json, Qt6, ORT) |
| Управление фаерволом | netsh advfirewall (Windows) | — | NFR-002 | Стандартный CLI-инструмент Windows Firewall. Вызывается через `QProcess::startDetached()` |

"""
            text = text[:start_1_5] + t1_5_block + text[end_1_5:]

    # Dedup rows in all tables
    lines = text.splitlines()
    new_lines = []
    prev_line = ""
    for line in lines:
        if line.strip() == "|" or line.strip() == "---":
            continue
        if line.startswith("|") and line == prev_line:
            continue
        new_lines.append(line)
        prev_line = line
    text = "\n".join(new_lines)

    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(text)

fix_all_table_glitches('docs/academic/ВКР_черновик_claude.md')
print('Tables fixed.')
