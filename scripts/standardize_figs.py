import re

def final_figure_fix_v2(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        text = f.read()

    # Match <p align="center">(Рис\.|Рисунок) Number. Title</p>
    pattern = r'<p align="center">(?:Рис\.|Рисунок)\s+([0-9А-Я]+(?:\.[0-9]+)*)\.?\s*(.*?)(?:</p>|\n)'
    
    def caption_replacer(match):
        num = match.group(1)
        title = match.group(2).strip()
        # Clean title from trailing dot
        if title.endswith('.'):
            title = title[:-1]
        return f'<p align="center">Рисунок {num}. {title}</p>'

    text = re.sub(pattern, caption_replacer, text)

    # Specific fix for Figure 4.1 if missed
    text = text.replace('<p align="center">Рисунок 4.1. ROC-кривая классификаторов\n', '<p align="center">Рисунок 4.1. ROC-кривая классификаторов</p>\n')
    
    # Ensure no triple dot or other artifacts in numbers
    text = re.sub(r'Рисунок\s+(\d+\.\d+)\.\d+\.', r'Рисунок \1.', text)

    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(text)

final_figure_fix_v2('docs/academic/ВКР_черновик_claude.md')
print('Standardized all figure captions to "Рисунок X.Y. Title".')
