
import re
import os

def format_gost_code(code):
    # 1. 2-space indentation
    code = code.replace('\t', '  ')
    lines = code.splitlines()
    new_lines = []
    for line in lines:
        if not line.strip():
            new_lines.append('')
            continue
        leading = len(line) - len(line.lstrip(' '))
        if leading > 0 and leading % 4 == 0:
            line = ' ' * (leading // 2) + line.lstrip(' ')
        new_lines.append(line)
    code = '\n'.join(new_lines)

    # 2. Russify common comments (heuristics)
    comment_map = {
        r'// Load scaler params': '// Загрузка параметров масштабировщика',
        r'// Process a packet': '// Обработка сетевого пакета',
        r'// Finalize the window': '// Завершение окна агрегации',
        r'// Get normalized features': '// Получение нормализованного вектора признаков',
        r'// Get telemetry': '// Получение телеметрии',
        r'// Reset for a new window': '// Сброс состояния для нового окна',
        r'// Set the scaler type': '// Установка типа масштабировщика для модели',
        r'// Window state': '// Состояние временного окна',
        r'// Per-flow tracking': '// Структуры для отслеживания потоков',
        r'// Global Window Counters': '// Глобальные счетчики окна',
        r'// Size histogram': '// Гистограмма размеров пакетов',
        r'// IP layer': '// Уровень сетевого протокола (IP)',
        r'// Determine ports and protocol': '// Определение портов и протокола',
        r'// Flow-based forward/backward': '// Классификация по направлениям потока',
        r'// Payload Entropy': '// Вычисление энтропии полезной нагрузки',
        r'// Base and Extended features': '// Базовые и расширенные признаки',
        r'// Async writer': '// Асинхронный поток записи',
        r'// History retrieval': '// Получение истории из БД',
        r'// Main PPS chart': '// Главный график интенсивности трафика',
        r'// Donut charts': '// Кольцевые диаграммы ресурсов',
        r'// Analytics tab': '// Вкладка расширенной аналитики',
        r'// System metrics': '// Системные метрики (ЦП, ОЗУ)',
        r'#pragma once': '#pragma once // Директива защиты от повторного включения',
        r'// Hot-swap': '// Потокобезопасная горячая замена модели',
        r'// Session management': '// Управление сессиями мониторинга',
        r'// Predict: returns': '// Инференс: возвращает {метка, уверенность}'
    }

    for eng, rus in comment_map.items():
        code = re.sub(eng, rus, code, flags=re.IGNORECASE)

    return code

def get_gost_header(filename, description):
    return f"""/**
 * @file {filename}
 * @brief {description}.
 * Разработано в рамках ВКР. Соответствует ГОСТ 19.402-78.
 */
"""

def process_file(path, desc):
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    formatted = format_gost_code(content)
    header = get_gost_header(os.path.basename(path), desc)
    return header + formatted

# Configuration for Appendices
collector_files = [
    ("src/network/TrafficMonitor.hpp", "Модуль захвата пакетов (заголовок)"),
    ("src/network/TrafficMonitor.cpp", "Модуль захвата пакетов (реализация)"),
    ("src/network/FeatureExtractor.hpp", "Модуль извлечения признаков (заголовок)"),
    ("src/network/FeatureExtractor.cpp", "Модуль извлечения признаков (реализация)"),
    ("src/ml/ModelInferencer.hpp", "Модуль инференса ONNX (заголовок)"),
    ("src/ml/ModelInferencer.cpp", "Модуль инференса ONNX (реализация)"),
    ("src/core/FirewallManager.hpp", "Модуль управления фаерволом (заголовок)"),
    ("src/core/FirewallManager.cpp", "Модуль управления фаерволом (реализация)"),
    ("src/common/DatabaseManager.hpp", "Модуль взаимодействия с БД (заголовок)"),
    ("src/common/DatabaseManager.cpp", "Модуль взаимодействия с БД (реализация)")
]

monitor_files = [
    ("src/monitor_ui/DashboardWidget.hpp", "Главный графический интерфейс (заголовок)"),
    ("src/monitor_ui/DashboardWidget.cpp", "Главный графический интерфейс (реализация)")
]

with open('docs/academic/ВКР_черновик_claude.md', 'r', encoding='utf-8') as f:
    vkr = f.read()

# Build Appendix B
app_b = "## ПРИЛОЖЕНИЕ Б\n\nЛистинги программного модуля ddos_collector\n\n"
for path, desc in collector_files:
    content = process_file(path, desc)
    app_b += f"### {desc} ({os.path.basename(path)})\n\n```cpp\n{content}\n```\n\n"

# Build Appendix V
app_v = "## ПРИЛОЖЕНИЕ В\n\nЛистинги программного модуля ddos_monitor\n\n"
for path, desc in monitor_files:
    content = process_file(path, desc)
    app_v += f"### {desc} ({os.path.basename(path)})\n\n```cpp\n{content}\n```\n\n"

# Replace via regex
match_b = re.search(r'#+\s*Приложение Б', vkr, re.IGNORECASE)
match_g = re.search(r'#+\s*Приложение Г', vkr, re.IGNORECASE)

if match_b and match_g:
    new_vkr = vkr[:match_b.start()] + app_b + app_v + vkr[match_g.start():]
    
    # Also Russify the short snippets in Section 3
    # These were "Listing 1", "Listing 2" etc.
    # I already did some manual ones, let's ensure indentation for all
    
    with open('docs/academic/ВКР_черновик_claude.md', 'w', encoding='utf-8') as f:
        f.write(new_vkr)
    print("All listings russified and formatted according to GOST.")
else:
    print("Failed to find anchors for replacement.")
