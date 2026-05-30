
import sys

def format_code(content):
  # Replace tabs with 2 spaces
  content = content.replace('\t', '  ')
  # Ensure indentation is 2 spaces (basic heuristic)
  # Most of the code already seems to use 4 spaces or mix, 
  # but the tool output suggests 4. I'll transform 4 to 2.
  lines = content.splitlines()
  new_lines = []
  for line in lines:
    leading_spaces = len(line) - len(line.lstrip(' '))
    if leading_spaces % 4 == 0:
      new_line = ' ' * (leading_spaces // 2) + line.lstrip(' ')
      new_lines.append(new_line)
    else:
      new_lines.append(line)
  return '\n'.join(new_lines)

def generate_listing(title, filename, content):
  formatted = format_code(content)
  return f"### {title} ({filename})\n\n```cpp\n{formatted}\n```\n\n"

# Files for Application B (Collector)
files_b = [
    ("Модуль захвата пакетов (заголовок)", "src/network/TrafficMonitor.hpp"),
    ("Модуль захвата пакетов (реализация)", "src/network/TrafficMonitor.cpp"),
    ("Модуль ML-инференса (заголовок)", "src/ml/ModelInferencer.hpp"),
    ("Модуль ML-инференса (реализация)", "src/ml/ModelInferencer.cpp"),
    ("Модуль управления фаерволом (заголовок)", "src/core/FirewallManager.hpp"),
    ("Модуль управления фаерволом (реализация)", "src/core/FirewallManager.cpp"),
    ("Модуль работы с БД (заголовок)", "src/common/DatabaseManager.hpp"),
    ("Модуль работы с БД (реализация)", "src/common/DatabaseManager.cpp")
]

# Files for Application V (Monitor)
files_v = [
    ("Главный виджет мониторинга (заголовок)", "src/monitor_ui/DashboardWidget.hpp"),
    ("Главный виджет мониторинга (реализация)", "src/monitor_ui/DashboardWidget.cpp")
]

with open('docs/academic/ВКР_черновик_claude.md', 'r', encoding='utf-8') as f:
    vkr = f.read()

app_b_content = "## ПРИЛОЖЕНИЕ Б\n\nЛистинги программного модуля ddos_collector\n\n"
for title, path in files_b:
    with open(path, 'r', encoding='utf-8') as f:
        app_b_content += generate_listing(title, path.split('/')[-1], f.read())

app_v_content = "## ПРИЛОЖЕНИЕ В\n\nЛистинги программного модуля ddos_monitor\n\n"
for title, path in files_v:
    with open(path, 'r', encoding='utf-8') as f:
        app_v_content += generate_listing(title, path.split('/')[-1], f.read())

# Replace the placeholders in the main document
vkr = vkr.replace('### Приложение Б. Листинг программы ddos_collector (ключевые модули)\n\n*(Полные листинги: TrafficMonitor, FeatureExtractor, ModelInferencer, DetectionEngine, FirewallManager, DatabaseManager)*', app_b_content)
vkr = vkr.replace('### Приложение В. Листинг программы ddos_monitor (ключевые модули)\n\n*(Полные листинги: DashboardWidget, EventHistoryWidget, ReportGenerator)*', app_v_content)

with open('docs/academic/ВКР_черновик_claude.md', 'w', encoding='utf-8') as f:
    f.write(vkr)

print("Listings for Application B and V generated successfully.")
