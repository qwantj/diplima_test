import re

def check_captions(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        text = f.read()

    # Find all Рисунок captions
    captions = list(re.finditer(r'<p align="center">Рисунок (.*?)</p>', text))
    for c in captions:
        pos = c.start()
        # Look at 2000 chars before the caption
        preceding = text[max(0, pos-2000):pos]
        # A figure caption should follow a mermaid block or an image
        if '```mermaid' in preceding or '![' in preceding:
             # It likely follows the content
             pass
        else:
             print(f"Potential issue: caption '{c.group(0)}' might be ABOVE or far from content.")

check_captions('docs/academic/ВКР_черновик_claude.md')
