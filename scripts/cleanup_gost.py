import re
import sys

def fix_all_formatting_v5(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        text = f.read()

    # 1. Identify Sections accurately
    # sections is a list of (start_pos, name)
    sections = []
    for m in re.finditer(r'^## (\d+)\.', text, re.MULTILINE):
        sections.append((m.start(), m.group(1)))
    
    # Add terminator for the last section
    # Search for start of Appendices or Bibliography
    end_of_main = re.search(r'^## (?:ПРИЛОЖЕНИЯ|СПИСОК|ЗАКЛЮЧЕНИЕ)', text, re.MULTILINE)
    if end_of_main:
        sections.append((end_of_main.start(), 'END'))
    else:
        sections.append((len(text), 'END'))
    
    sections.sort()

    def get_sec_num(pos):
        for i in range(len(sections)-1):
            if sections[i][0] <= pos < sections[i+1][0]:
                return sections[i][1]
        return "0"

    # 2. Find and Renumber Headers
    table_headers = list(re.finditer(r'<p align="right">Таблица \d+(?:\.\d+)*</p>', text))
    fig_headers = list(re.finditer(r'Рис\.\s+\d+(?:\.\d+)*\.', text))
    
    sec_table_counts = {}
    sec_fig_counts = {}
    header_replacements = [] # (start, end, new_text)

    for m in table_headers:
        s = get_sec_num(m.start())
        if s in ["0", "END"]: continue
        sec_table_counts[s] = sec_table_counts.get(s, 0) + 1
        new_num = f"{s}.{sec_table_counts[s]}"
        header_replacements.append((m.start(), m.end(), f'<p align="right">Таблица {new_num}</p>'))

    for m in fig_headers:
        s = get_sec_num(m.start())
        if s in ["0", "END"]: continue
        sec_fig_counts[s] = sec_fig_counts.get(s, 0) + 1
        new_num = f"{s}.{sec_fig_counts[s]}"
        header_replacements.append((m.start(), m.end(), f'Рис. {new_num}.'))

    # Apply replacements in reverse
    header_replacements.sort(key=lambda x: x[0], reverse=True)
    for start, end, new_h in header_replacements:
        text = text[:start] + new_h + text[end:]

    # 3. Fix broken table rows
    lines = text.splitlines()
    fixed_lines = []
    for line in lines:
        if line.strip().startswith('|') and line.strip().endswith('|'):
            if not re.search(r'[a-zA-Zа-яА-Я0-9]', line):
                # Separator row
                line = re.sub(r'[\s—]', '', line)
                fixed_lines.append(line)
            else:
                # Data row: fix empty cells
                line = line.replace('| |', '| — |').replace('|  |', '| — |')
                fixed_lines.append(line)
        else:
            fixed_lines.append(line)
    text = '\n'.join(fixed_lines)

    # 4. Clean up reference formatting (avoid double numbers)
    # Correct any табл. X.Y.Z back to табл. X.Y
    text = re.sub(r'табл\.\s+(\d+)\.(\d+)(?:\.\d+)+', r'табл. \1.\2', text)
    text = re.sub(r'[Рр]ис\.\s+(\d+)\.(\d+)(?:\.\d+)+', r'рис. \1.\2', text)

    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(text)

fix_all_formatting_v5('docs/academic/ВКР_черновик_claude.md')
print('Success.')
