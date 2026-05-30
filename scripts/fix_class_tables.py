import re

with open('docs/academic/ВКР Черновик 2.md', 'r', encoding='utf-8') as f:
    ref_text = f.read()

with open('docs/academic/ВКР_черновик_claude.md', 'r', encoding='utf-8') as f:
    draft_text = f.read()

# We need to extract the tables from ref_text and put them in draft_text.
# The tables we care about are for:
# TrafficMonitor, FeatureExtractor, ModelInferencer, DetectionEngine, DatabaseManager, FirewallManager
# MainWindow, TcpClient

classes = [
    'TrafficMonitor', 'FeatureExtractor', 'ModelInferencer', 
    'DetectionEngine', 'DatabaseManager', 'FirewallManager',
    'MainWindow', 'TcpClient'
]

for cls in classes:
    # Find table in ref_text
    # The header in ref_text is like:
    # <p align="right">Таблица 10</p>
    # <p align="center"><b>Поля и методы класса TrafficMonitor</b></p>
    # or something similar.
    # Note: In ref_text, it might have ** ** around the tags. Let's use a flexible regex.
    
    ref_pattern = r'<p align="center"><b>Поля и методы класса ' + cls + r'\*?\*?</b></p>[\r\n]+(\| Имя \| Тип \| Описание \| Входные параметры \| Выходное значение \|[\r\n]+\|---\|---\|---\|---\|---\|[\r\n]+(?:\|.*?\|[\r\n]+)+)'
    
    ref_match = re.search(ref_pattern, ref_text)
    if not ref_match:
        print(f"Could not find ref table for {cls}")
        continue
        
    ref_table_content = ref_match.group(1).strip()
    
    # Find table in draft_text
    # Header:
    # <p align="center"><b>Поля и методы класса TrafficMonitor</b></p>
    draft_pattern = r'(<p align="center"><b>Поля и методы класса ' + cls + r'</b></p>\n\n)\| Имя \| Тип \| Описание \| Входные параметры \| Выходное значение \|\n\|---\|---\|---\|---\|---\|\n(?:\|.*?\|\n)+'
    
    def repl(m):
        return m.group(1) + ref_table_content + '\n'
        
    new_draft_text = re.sub(draft_pattern, repl, draft_text)
    
    if new_draft_text != draft_text:
        draft_text = new_draft_text
        print(f"Updated table for {cls}")
    else:
        print(f"Failed to update table for {cls}")

with open('docs/academic/ВКР_черновик_claude.md', 'w', encoding='utf-8') as f:
    f.write(draft_text)

print("Done.")
