import re

with open('docs/academic/ВКР_черновик_claude.md', 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('<p align="center">Рисунок ', '<p align="center">Рис. ')

with open('docs/academic/ВКР_черновик_claude.md', 'w', encoding='utf-8') as f:
    f.write(content)

print("Done.")
