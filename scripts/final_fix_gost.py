import re

def fix_citations_final(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        text = f.read()

    # Section 1.1 table and text citations
    text = text.replace('табл. 1.1 [21]', 'табл. 1.1 [28]')
    text = text.replace('эффект усиления, пропорциональный числу хостов [21]', 'эффект усиления, пропорциональный числу хостов [28]')
    text = text.replace('определения набора признаков классификации [21]', 'определения набора признаков классификации [28]')
    text = text.replace('целью разрабатываемого комплекса. [21]', 'целью разрабатываемого комплекса. [28]')

    # Section 1.3
    text = text.replace('встроенный Qt6-монитор «из коробки» ... [21]', 'встроенный Qt6-монитор «из коробки» ... [23]')
    text = text.replace('автономный контроль над защищаемым периметром сети [21]', 'автономный контроль над защищаемым периметром сети [28]')

    # Section 1.4.1
    text = text.replace('F1-score = 0,9997 на датасете CIC-DDoS2019 [21]', 'F1-score = 0,9997 на датасете CIC-DDoS2019 [24]')
    
    # Definitions section (check if any [21] left that are wrong)
    # I already fixed most in previous script but let's be sure.

    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(text)

fix_citations_final('docs/academic/ВКР_черновик_claude.md')
print('Citations updated.')
