import re

def final_bibliography_polish(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        text = f.read()

    # Match bibliography start
    bib_marker = "## СПИСОК ИСПОЛЬЗОВАННОЙ ЛИТЕРАТУРЫ"
    start_pos = text.find(bib_marker)
    if start_pos == -1: return

    # Correct entries 11, 31, 32 to include the weird brackets if present in user list
    # and fix the double colon in 31, 32
    text = text.replace('11) ISO/IEC 14882:2020. Programming', '11) ISO/IEC 14882:2020 [11]. Programming')
    text = text.replace('31) ГОСТ 19.301-79. Единая', '31) ГОСТ 19.301-79 [31] [31]. Единая')
    text = text.replace('32) ГОСТ 19.401-78. Единая', '32) ГОСТ 19.401-78 [32] [32]. Единая')
    text = text.replace(': : непосредственный', ': непосредственный')

    # Ensure Section 1.1 table note is correct
    text = text.replace('на основе данных [3, 9, 21]', 'на основе данных [3, 9, 28]')

    # Fix Figure 4.1 caption closing tag (saw it in previous thought but let's be sure)
    text = re.sub(r'<p align="center">Рисунок 4.1. ROC-кривая классификаторов(?!\s*</p>)', r'<p align="center">Рисунок 4.1. ROC-кривая классификаторов</p>', text)

    # Fix Section 1.1 table re-duplication if any
    # I saw | ... | followed by --- then | ... | again.
    # Actually, I'll just look for triple-dashes between table rows.
    text = re.sub(r'\|\n---\n\|', '|\n|', text)

    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(text)

final_bibliography_polish('docs/academic/ВКР_черновик_claude.md')
print('Final polish applied.')
