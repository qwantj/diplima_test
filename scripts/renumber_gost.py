import re

def fix_gost_v2(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        text = f.read()

    # 1. Identify Sections
    section_patterns = [
        (r'^## 1\.', '1'),
        (r'^## 2\.', '2'),
        (r'^## 3\.', '3'),
        (r'^## 4\.', '4'),
    ]
    sec_pos = []
    for pattern, name in section_patterns:
        for m in re.finditer(pattern, text, re.MULTILINE):
            sec_pos.append((m.start(), name))
    sec_pos.sort()
    sec_pos.append((len(text), 'EOF'))

    def get_sec(pos):
        for i in range(len(sec_pos)-1):
            if sec_pos[i][0] <= pos < sec_pos[i+1][0]:
                return sec_pos[i][1]
        return '0'

    # 2. Collect Objects
    # Tables
    tables = []
    # Match <p align="right">Таблица (\d+(?:\.\d+)*)</p>
    for m in re.finditer(r'<p align="right">Таблица \d+(?:\.\d+)*</p>', text):
        tables.append({'type': 'table', 'start': m.start(), 'end': m.end()})

    # Figures
    figs = []
    # Match Рис\. \d+(?:\.\d+)*\.
    # Be careful not to match refs here. Usually headers are at the start of a line or after a newline.
    # But in this doc they are often in the middle of text or centered.
    # Let's assume Рис. X. is a header if it has a period after the number.
    for m in re.finditer(r'Рис\.\s+\d+(?:\.\d+)*\.', text):
        figs.append({'type': 'fig', 'start': m.start(), 'end': m.end()})

    # 3. Renumber
    table_map = {} # old -> new
    fig_map = {}
    sec_table_counts = {}
    sec_fig_counts = {}

    # To build the map, we need to know the old numbers.
    # Let's extract them.
    for t in tables:
        old_num = re.search(r'Таблица (\d+(?:\.\d+)*)', text[t['start']:t['end']]).group(1)
        sec = get_sec(t['start'])
        if sec == '0': continue
        sec_table_counts[sec] = sec_table_counts.get(sec, 0) + 1
        new_num = f"{sec}.{sec_table_counts[sec]}"
        t['new_num'] = new_num
        table_map[old_num] = new_num

    for f in figs:
        old_num = re.search(r'Рис\.\s+(\d+(?:\.\d+)*)\.', text[f['start']:f['end']]).group(1)
        sec = get_sec(f['start'])
        if sec == '0': continue
        sec_fig_counts[sec] = sec_fig_counts.get(sec, 0) + 1
        new_num = f"{sec}.{sec_fig_counts[sec]}"
        f['new_num'] = new_num
        fig_map[old_num] = new_num

    # 4. Apply Header Replacements (Reverse Order)
    all_items = tables + figs
    all_items.sort(key=lambda x: x['start'], reverse=True)
    
    new_text = text
    for item in all_items:
        if 'new_num' not in item: continue
        s, e = item['start'], item['end']
        if item['type'] == 'table':
            repl = f'<p align="right">Таблица {item["new_num"]}</p>'
        else:
            repl = f'Рис. {item["new_num"]}.'
        new_text = new_text[:s] + repl + new_text[e:]

    # 5. Update References
    # Use a single pass for each type to avoid double-replacement.
    # Sort old numbers by length descending.
    
    def update_text_refs(content, mapping, prefix_regex):
        sorted_old = sorted(mapping.keys(), key=len, reverse=True)
        # Use a placeholder to avoid overlapping replacements
        for i, old in enumerate(sorted_old):
            pattern = rf'{prefix_regex}\s+{re.escape(old)}(?!\.\d)'
            content = re.sub(pattern, f'__REF_{i}__', content)
        
        for i, old in enumerate(sorted_old):
            content = content.replace(f'__REF_{i}__', f'табл. {mapping[old]}' if 'табл' in prefix_regex else f'рис. {mapping[old]}')
        return content

    new_text = update_text_refs(new_text, table_map, 'табл\.')
    new_text = update_text_refs(new_text, fig_map, '[Рр]ис\.')

    # 6. Fix broken table rows (remove ' — ' from separators)
    # A separator row looks like |---|---|
    new_text = re.sub(r'\|\s*—\s*\|---', '|---', new_text)
    new_text = re.sub(r'---\|\s*—\s*\|', '---|', new_text)
    # And specifically the mess from previous run:
    new_text = re.sub(r'\|\s*—\s*\|', '| — |', new_text) # Normalize
    new_text = re.sub(r'\|\s*—\s*\|---|---\|\s*—\s*\|', lambda m: m.group(0).replace(' — ', ''), new_text)
    
    # Let's just fix the whole separator pattern: |---|---|
    new_text = re.sub(r'\|\s*—\s*---', '|---', new_text)
    new_text = re.sub(r'---\s*—\s*\|', '---|', new_text)

    # 7. Final check on empty cells in data rows
    lines = new_text.splitlines()
    fixed_lines = []
    for line in lines:
        if line.startswith('|') and line.endswith('|'):
            if not re.search(r'[a-zA-Zа-яА-Я0-9]', line): # separator line
                fixed_lines.append(line.replace(' — ', ''))
            else:
                # Replace empty cells
                line = line.replace('| |', '| — |').replace('|  |', '| — |')
                fixed_lines.append(line)
        else:
            fixed_lines.append(line)
    
    new_text = '\n'.join(fixed_lines)

    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(new_text)

fix_gost_v2('docs/academic/ВКР_черновик_claude.md')
print('Corrected table renumbering and structure.')
