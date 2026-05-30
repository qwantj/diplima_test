
import re

def format_gost(code):
    code = code.replace('\t', '  ')
    lines = code.splitlines()
    new_lines = []
    for line in lines:
        if not line.strip():
            new_lines.append('')
            continue
        leading = len(line) - len(line.lstrip(' '))
        if leading > 0 and leading % 4 == 0:
            new_lines.append(' ' * (leading // 2) + line.lstrip(' '))
        else:
            new_lines.append(line)
    return '\n'.join(new_lines)

def add_gost_comments(code, filename):
    header = f"""/**
 * @file {filename}
 * @brief Программная реализация модуля системы обнаружения DDoS-атак.
 * Оформлено в соответствии с требованиями ГОСТ 19.402-78.
 */
"""
    return header + code

def get_file_content(path):
    with open(path, 'r', encoding='utf-8') as f:
        return f.read()

collector_files = [
    ("network/TrafficMonitor.hpp", "Модуль захвата трафика (заголовок)"),
    ("network/TrafficMonitor.cpp", "Модуль захвата трафика (реализация)"),
    ("network/FeatureExtractor.hpp", "Модуль извлечения признаков (заголовок)"),
    ("network/FeatureExtractor.cpp", "Модуль извлечения признаков (реализация)"),
    ("ml/ModelInferencer.hpp", "Модуль инференса (заголовок)"),
    ("ml/ModelInferencer.cpp", "Модуль инференса (реализация)"),
    ("core/FirewallManager.hpp", "Модуль управления фаерволом (заголовок)"),
    ("core/FirewallManager.cpp", "Модуль управления фаерволом (реализация)"),
    ("common/DatabaseManager.hpp", "Модуль базы данных (заголовок)"),
    ("common/DatabaseManager.cpp", "Модуль базы данных (реализация)")
]

monitor_files = [
    ("monitor_ui/DashboardWidget.hpp", "Главный экран мониторинга (заголовок)"),
    ("monitor_ui/DashboardWidget.cpp", "Главный экран мониторинга (реализация)")
]

with open('docs/academic/ВКР_черновик_claude.md', 'r', encoding='utf-8') as f:
    vkr = f.read()

app_b = "## ПРИЛОЖЕНИЕ Б\n\nЛистинги программного модуля ddos_collector\n\n"
for path, desc in collector_files:
    raw = get_file_content("src/" + path)
    app_b += f"### {desc} ({path.split('/')[-1]})\n\n```cpp\n{add_gost_comments(format_gost(raw), path.split('/')[-1])}\n```\n\n"

app_v = "## ПРИЛОЖЕНИЕ В\n\nЛистинги программного модуля ddos_monitor\n\n"
for path, desc in monitor_files:
    raw = get_file_content("src/" + path)
    app_v += f"### {desc} ({path.split('/')[-1]})\n\n```cpp\n{add_gost_comments(format_gost(raw), path.split('/')[-1])}\n```\n\n"

# Use regex to find anchors regardless of depth (## or ###) and case
match_b = re.search(r'#+\s*Приложение Б', vkr, re.IGNORECASE)
match_g = re.search(r'#+\s*Приложение Г', vkr, re.IGNORECASE)

if match_b and match_g:
    start_b = match_b.start()
    start_g = match_g.start()
    new_vkr = vkr[:start_b] + app_b + app_v + vkr[start_g:]
    with open('docs/academic/ВКР_черновик_claude.md', 'w', encoding='utf-8') as f:
        f.write(new_vkr)
    print("Full GOST listings injected successfully.")
else:
    print(f"Anchors not found. B: {bool(match_b)}, G: {bool(match_g)}")
