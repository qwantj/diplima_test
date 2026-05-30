
with open('docs/academic/ВКР_черновик_claude.md', 'r', encoding='utf-8') as f:
    text = f.read()

old_block = 'ERR["AppLogger::error()\\nabort"]'
new_block = 'ERR["AppLogger::error()\\nreturn"]\n    ERR --> Z'

text = text.replace(old_block, new_block)

with open('docs/academic/ВКР_черновик_claude.md', 'w', encoding='utf-8') as f:
    f.write(text)

print('Mermaid ERR block fixed and linked to Z.')
