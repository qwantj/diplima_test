import re

def sort_bibliography(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        text = f.read()

    bib_start = text.rfind('## СПИСОК ИСПОЛЬЗОВАННОЙ ЛИТЕРАТУРЫ')
    appendices_start = text.find('<p align="center">ПРИЛОЖЕНИЕ А</p>', bib_start)
    if appendices_start == -1:
        appendices_start = text.find('## ПРИЛОЖЕНИЕ А', bib_start)

    if bib_start == -1 or appendices_start == -1:
        print('Markers not found.', bib_start, appendices_start)
        return

    bib_content = text[bib_start:appendices_start]
    
    items = re.findall(r'^(\d+)\)\s+(.*)', bib_content, re.MULTILINE)
    
    russian_items = []
    latin_items = []
    
    for num, content in items:
        match = re.search(r'[a-zA-Zа-яА-ЯёЁ]', content)
        if match:
            char = match.group(0).upper()
            if 'А' <= char <= 'Я' or char == 'Ё':
                russian_items.append((num, content))
            else:
                latin_items.append((num, content))
        else:
            latin_items.append((num, content))
            
    russian_items.sort(key=lambda x: x[1])
    latin_items.sort(key=lambda x: x[1])
    
    all_sorted = russian_items + latin_items
    
    old_to_new = {}
    new_bib_lines = ['## СПИСОК ИСПОЛЬЗОВАННОЙ ЛИТЕРАТУРЫ\n\n*(Оформление по ГОСТ Р 7.0.100–2018)*\n']
    
    for new_idx, (old_num, content) in enumerate(all_sorted, 1):
        old_to_new[str(old_num)] = str(new_idx)
        content = content.replace('ISO/IEC 14882:2020 [11]. Programming', 'ISO/IEC 14882:2020. Programming')
        content = content.replace('ГОСТ 19.301-79 [31] [31].', 'ГОСТ 19.301-79.')
        content = content.replace('ГОСТ 19.401-78 [32] [32].', 'ГОСТ 19.401-78.')
        content = content.replace('https://www.postgresql.org/docs/current/sql-copy.html', 'https://libpqxx.readthedocs.io/')
        
        new_bib_lines.append(f'{new_idx}) {content}\n')
        
    new_bib = '\n'.join(new_bib_lines) + '\n*(Скан/копия задания)*\n\n'
    
    text = text[:bib_start] + new_bib + text[appendices_start:]
    
    def replacer(match):
        inner = match.group(1)
        if 'N' in inner:
            return match.group(0)
            
        parts = re.split(r'(\D+)', inner)
        res = ''
        for p in parts:
            if p.isdigit() and p in old_to_new:
                res += old_to_new[p]
            else:
                res += p
        return f'[{res}]'
        
    text = re.sub(r'\[([0-9\s,–-]+)\]', replacer, text)
    
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(text)
    print('Sorted and references updated successfully.')

sort_bibliography('docs/academic/ВКР_черновик_claude.md')
