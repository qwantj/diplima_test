import re

def process_file():
    filepath = 'docs/academic/ВКР_черновик_claude.md'
    with open(filepath, 'r', encoding='utf-8') as f:
        text = f.read()

    app_b_start = text.find('## ПРИЛОЖЕНИЕ Б')
    app_g_start = text.find('## ПРИЛОЖЕНИЕ Г')

    if app_b_start == -1 or app_g_start == -1:
        print("Could not find appendices")
        return

    part1 = text[:app_b_start]
    apps = text[app_b_start:app_g_start]
    part_end = text[app_g_start:]

    def clean_cpp_blocks(app_text):
        pattern = r'(```cpp\n)(.*?)(\n```)'
        
        def replacer(match):
            prefix = match.group(1)
            code = match.group(2)
            suffix = match.group(3)
            
            # Сжимаем вообще любые комбинации \n и пробелов/табов до максимум 2 переносов
            code = re.sub(r'\n\s*\n', '\n\n', code)
            code = re.sub(r'\n{3,}', '\n\n', code)
            
            return prefix + code.strip() + suffix
            
        return re.sub(pattern, replacer, app_text, flags=re.DOTALL)

    apps = clean_cpp_blocks(apps)
    # И для всего текста приложения тоже
    apps = re.sub(r'\n\s*\n', '\n\n', apps)
    apps = re.sub(r'\n{3,}', '\n\n', apps)

    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(part1 + apps + part_end)
    print("Done")

if __name__ == '__main__':
    process_file()