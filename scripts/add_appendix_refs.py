import re

def add_appendix_references(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        text = f.read()

    # Ref to E (Graphics) in 2.1
    text = text.replace(
        'Общая структурная схема программного комплекса представлена на рис. 2.',
        'Общая структурная схема программного комплекса представлена на рис. 2, а дополнительные графические материалы приведены в приложении Е.'
    )

    # Ref to G (DB) in 2.3
    text = text.replace(
        'На рис. 10 представлена логическая схема базы данных в нотации ERD (Entity-Relationship Diagram)',
        'Полная схема структуры базы данных в виде SQL-скрипта представлена в приложении Г. На рис. 10 представлена логическая схема базы данных в нотации ERD (Entity-Relationship Diagram)'
    )

    # Ref to B and V (Code) in Section 3 intro
    text = text.replace(
        '## 3. РЕАЛИЗАЦИЯ ПРОГРАММНОГО КОМПЛЕКСА',
        '## 3. РЕАЛИЗАЦИЯ ПРОГРАММНОГО КОМПЛЕКСА\n\nВ данном разделе рассматриваются ключевые аспекты программной реализации комплекса. Полные исходные коды модулей подсистемы сбора приведены в приложении Б, подсистемы мониторинга — в приложении В.'
    )

    # Ref to D (Training) in 3.7
    text = text.replace(
        'Результат — файлы `model.onnx` и `scaler_params.json`, загружаемые в комплекс при старте без перекомпиляции.',
        'Результат — файлы `model.onnx` и `scaler_params.json`, загружаемые в комплекс при старте без перекомпиляции. Полный текст скрипта обучения на языке Python приведен в приложении Д.'
    )

    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(text)

add_appendix_references('docs/academic/ВКР_черновик_claude.md')
print('Added appendix references.')
