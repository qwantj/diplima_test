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
            code = re.sub(r'// Р¦РёРєР»: РёС‚РµСЂР°С†РёСЏ РїРѕ СЌР»РµРјРµРЅС‚Р°Рј\n*', '', code)
            code = re.sub(r'// Р’РµС‚РІР»РµРЅРёРµ: РїСЂРѕРІРµСЂРєР° Р»РѕРіРёС‡РµСЃРєРѕРіРѕ СѓСЃР»РѕРІРёСЏ\n*', '', code)
            code = re.sub(r'// Цикл: итерация по элементам\n*', '', code)
            code = re.sub(r'// Ветвление: проверка логического условия\n*', '', code)
            code = re.sub(r'// Назначение: локальная переменная.*?\n', '', code)
            code = re.sub(r'// РќР°Р·РЅР°С‡РµРЅРёРµ: Р»РѕРєР°Р»СЊРЅР°СЏ РїРµСЂРµРјРµРЅРЅР°СЏ.*?\n', '', code)
            
            # Сжимаем пустые строки
            code = re.sub(r'\n{3,}', '\n\n', code)
            
            return prefix + code + suffix
            
        return re.sub(pattern, replacer, app_text, flags=re.DOTALL)

    apps = clean_cpp_blocks(apps)
    # Сжимаем пустые строки еще раз глобально в приложениях
    apps = re.sub(r'\n{3,}', '\n\n', apps)

    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(part1 + apps + part_end)
    print("Done")

if __name__ == '__main__':
    process_file()