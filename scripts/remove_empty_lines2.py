import re

def process_file():
    filepath = 'docs/academic/ВКР_черновик_claude.md'
    with open(filepath, 'r', encoding='utf-8') as f:
        text = f.read()

    # Сначала уберем множественные переносы строк ВООБЩЕ ВЕЗДЕ в приложениях, а не только в cpp блоках, на всякий случай
    # И внутри cpp блоков еще раз прогоним
    
    app_b_start = text.find('## ПРИЛОЖЕНИЕ Б')
    app_g_start = text.find('## ПРИЛОЖЕНИЕ Г')

    if app_b_start == -1 or app_g_start == -1:
        print("Could not find appendices")
        return

    part1 = text[:app_b_start]
    apps = text[app_b_start:app_g_start]
    part_end = text[app_g_start:]

    # Убираем больше 2 переносов строки подряд во всем блоке приложений
    apps = re.sub(r'\n{3,}', '\n\n', apps)

    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(part1 + apps + part_end)
    print("Done")

if __name__ == '__main__':
    process_file()