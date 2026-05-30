import re

def final_eskd_cleanup(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        text = f.read()

    # 1. Fix Table 1.1 Duplication
    # Identify the section and replace the table part
    t1_1_header = '<p align="right">Таблица 1.1</p>\n<p align="center"><b>Классификация DoS/DDoS-атак по уровням OSI</b></p>'
    t1_1_content = """| Уровень OSI | Название | Пример атаки | Метод воздействия |
|---|---|---|---|
| Сетевой (L3) | Volumetric | ICMP Flood, Smurf | Исчерпание пропускной способности каналов. Амплификация (DNS, NTP, SSDP) даёт коэффициент усиления до 4096 раз [3] |
| Транспортный (L4) | Protocol | SYN Flood, UDP Flood | Исчерпание таблиц состояний соединений (state tables) сетевого оборудования [28] |
| Прикладной (L7) | Application | HTTP Flood, Slowloris | Имитация легитимных запросов; перегрузка прикладного ПО без генерации большого объёма трафика [9] |"""

    # Look for the header and then the table rows
    pattern = re.escape(t1_1_header) + r'\s+\|.*?\|'
    # Actually, I'll just find the header and replace everything from header till next \n\n
    start_pos = text.find(t1_1_header)
    if start_pos != -1:
        end_pos = text.find('\n\n', start_pos + len(t1_1_header) + 10)
        if end_pos != -1:
            text = text[:start_pos] + t1_1_header + "\n\n" + t1_1_content + text[end_pos:]

    # 2. Fix Table 1.2 (Search results table) - ensure no artifacts
    t1_2_header = '<p align="right">Таблица 1.2</p>\n<p align="center"><b>Сравнительный анализ существующих решений</b></p>'
    t1_2_content = """| Критерий | Snort / Suricata | Cloudflare | FastNetMon | Разрабатываемый комплекс |
|---|---|---|---|---|
| Метод обнаружения | Сигнатурный | Поведенческий + ML/DL | NetFlow-анализ | ML/DL (ONNX Runtime) |
| Реальное время | Да | Да | Да | Да |
| ML/DL-классификация | Нет | Да (закрытая) | Нет | Да (открытая ONNX) |
| Язык разработки | C | Проприетарный | C++ | C++17 |
| Автоблокировка | Только IPS mode | Да (Anycast) | Частично | Да (netsh) |
| Зависимость от сигнатур | Высокая | Низкая | Нет | Нет |
| Графический интерфейс | Нет | Веб-консоль | Через Grafana | Да (Qt6, встроенный) |
| Горячая замена модели | Нет | Н/А | Нет | Да |
| Контроль конфиденциальности | Полный | Ограниченный | Полный | Полный |"""

    start_pos = text.find(t1_2_header)
    if start_pos != -1:
        end_pos = text.find('\n\n', start_pos + len(t1_2_header) + 10)
        if end_pos != -1:
            text = text[:start_pos] + t1_2_header + "\n\n" + t1_2_content + text[end_pos:]

    # 3. Final Figure Placement Check
    # Ensure all captions are below the figure
    # We use <p align="center">Рисунок X.Y. Title</p>
    
    # Let's fix Figure 4.1 specifically (I saw it near TrafficMonitor listing in previous read)
    text = text.replace('<p align="center">Рисунок 4.1. ROC-кривая классификаторов</p>\n### 4.5. Нагрузочное тестирование', 
                        '```\n\n<p align="center">Рисунок 4.1. ROC-кривая классификаторов</p>\n\n### 4.5. Нагрузочное тестирование')

    # 4. Remove triple dots in captions
    text = re.sub(r'Рисунок\s+(\d+\.\d+)\.\d+\.', r'Рисунок \1.', text)
    text = re.sub(r'Таблица\s+(\d+\.\d+)\.\d+', r'Таблица \1', text)

    # 5. Fix "where" blocks for all formulas to be consistent
    # They should start with "где" and have semicolon at the end of rows.
    # I've done this for 1.1, 1.2, 1.3 already.

    # 6. Global Dash Replacement in Tables (empty cells)
    lines = text.splitlines()
    fixed_lines = []
    for line in lines:
        if line.strip().startswith('|') and line.strip().endswith('|'):
            if not re.search(r'[a-zA-Zа-яА-Я0-9]', line): # separator
                 fixed_lines.append(line.replace(' ', ''))
            else:
                 # Replace empty cells
                 line = line.replace('| |', '| — |').replace('|  |', '| — |')
                 fixed_lines.append(line)
        else:
            fixed_lines.append(line)
    text = '\n'.join(fixed_lines)

    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(text)

final_eskd_cleanup('docs/academic/ВКР_черновик_claude.md')
print('Final ESKD cleanup done.')
