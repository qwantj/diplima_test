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
            
            # Удаляем любые дублирующиеся комментарии, которые могли остаться из-за разных пробелов
            # Используем .encode('utf-8') и .decode('utf-8') для надежности если есть проблемы с кодировкой
            code = re.sub(r'// .*?\n', '\n', code) # Удаляем ВООБЩЕ ВСЕ однострочные комментарии в приложениях, кроме шапки
            
            # Восстанавливаем шапку
            header = """/*
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */\n"""
            code = re.sub(r'/\*.*?\*/\n*', '', code, flags=re.DOTALL) # Удаляем старую шапку
            
            # Сжимаем пустые строки
            code = re.sub(r'\n{3,}', '\n\n', code)
            
            # Убираем пробелы в пустых строках
            code = re.sub(r'\n[ \t]+\n', '\n\n', code)
            
            return prefix + header + code.strip() + suffix
            
        return re.sub(pattern, replacer, app_text, flags=re.DOTALL)

    apps = clean_cpp_blocks(apps)
    # Сжимаем пустые строки еще раз глобально в приложениях
    apps = re.sub(r'\n{3,}', '\n\n', apps)
    apps = re.sub(r'\n[ \t]+\n', '\n\n', apps)

    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(part1 + apps + part_end)
    print("Done")

if __name__ == '__main__':
    process_file()