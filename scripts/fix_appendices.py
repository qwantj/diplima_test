import re

def fix_vkr_appendices(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        text = f.read()

    # 1. Update Annotation
    text = re.sub(
        r'12 рисунков, 30 таблиц и 5 приложений',
        '18 рисунков, 55 таблиц и 6 приложений',
        text
    )
    text = re.sub(
        r'12 figures, 30 tables and 5 appendices',
        '18 figures, 55 tables and 6 appendices',
        text
    )

    # 2. Update TOC
    toc_replacement = """ПРИЛОЖЕНИЯ ........................................................................................................................ [N]
   ПРИЛОЖЕНИЕ А Задание на выпускную квалификационную работу ......................... [N]
   ПРИЛОЖЕНИЕ Б Листинги программного модуля ddos_collector ............................... [N]
   ПРИЛОЖЕНИЕ В Листинги программного модуля ddos_monitor ................................. [N]
   ПРИЛОЖЕНИЕ Г Схема базы данных (SQL-скрипт) ......................................................... [N]
   ПРИЛОЖЕНИЕ Д Скрипт обучения модели XGBoost/ONNX (Python) ........................ [N]
   ПРИЛОЖЕНИЕ Е Графические материалы ....................................................................... [N]"""
    
    text = re.sub(r'ПРИЛОЖЕНИЯ \.* \[N\]', toc_replacement, text)

    # 3. Reformat Appendix Headers
    # Appendix A
    text = re.sub(
        r'### Приложение А\. (.*?)\n',
        r'<p align="center">ПРИЛОЖЕНИЕ А</p>\n<p align="center"><b>\1</b></p>\n',
        text
    )
    # Appendices B, V, G, D, E (Б, В, Г, Д, Е)
    designations = {
        'Б': 'Листинги программного модуля ddos_collector',
        'В': 'Листинги программного модуля ddos_monitor',
        'Г': 'Схема базы данных (SQL-скрипт)',
        'Д': 'Скрипт обучения модели XGBoost/ONNX (Python)',
        'Е': 'Графические материалы'
    }

    for letter, title in designations.items():
        pattern = rf'## ПРИЛОЖЕНИЕ {letter}\s*\n\n(.*?)\n'
        replacement = rf'<p align="center">ПРИЛОЖЕНИЕ {letter}</p>\n<p align="center"><b>{title}</b></p>\n'
        text = re.sub(pattern, replacement, text)

    # Remove extra section title if it exists uncentered
    text = text.replace('## ПРИЛОЖЕНИЯ', '<p align="center"><b>ПРИЛОЖЕНИЯ</b></p>')

    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(text)

fix_vkr_appendices('docs/academic/ВКР_черновик_claude.md')
print('Fixed appendices formatting.')
