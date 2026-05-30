import re

def ensure_page_breaks_before_appendices(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        text = f.read()

    # Find all "<p align="center">ПРИЛОЖЕНИЕ [А-Я]"
    # and ensure there's a "---" before it if not already present.
    
    # First, handle the main section header
    text = re.sub(r'(?<!\n\n)---\n\n<p align="center"><b>ПРИЛОЖЕНИЯ</b></p>', r'\n\n---\n\n<p align="center"><b>ПРИЛОЖЕНИЯ</b></p>', text)

    # For each specific appendix
    letters = 'АБВГДЕ'
    for letter in letters:
        pattern = rf'(?<!\n---\n\n)<p align="center">ПРИЛОЖЕНИЕ {letter}</p>'
        replacement = rf'\n---\n\n<p align="center">ПРИЛОЖЕНИЕ {letter}</p>'
        text = re.sub(pattern, replacement, text)

    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(text)

ensure_page_breaks_before_appendices('docs/academic/ВКР_черновик_claude.md')
print('Ensured page breaks before appendices.')
