
import re

with open('docs/academic/ВКР_черновик_claude.md', 'r', encoding='utf-8') as f:
    text = f.read()

# Fix all remaining decision blocks to be rhombuses instead of rectangles
# The pattern finds block_id["Text?"] and replaces with block_id{"Text?"}
def fix_decision(match):
    prefix = match.group(1)
    text_content = match.group(2)
    return f'{prefix}{{\"{text_content}\"}}'

text = re.sub(r'([A-Za-z0-9_]+)\["([^"]+\?)"\]', fix_decision, text)

with open('docs/academic/ВКР_черновик_claude.md', 'w', encoding='utf-8') as f:
    f.write(text)

print("Decision blocks fixed.")
