import re

def clean_refs(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        text = f.read()

    def replacer(match):
        content = match.group(1)
        # Remove page numbers like ", с. 145" or ", c. 145" (cyrillic/latin c)
        content = re.sub(r',\s*[сc]\.\s*\d+', '', content)
        # Replace semicolons with commas
        content = content.replace(';', ',')
        # Clean up double spaces or spaces after commas
        content = re.sub(r'\s*,\s*', ', ', content)
        return f'[{content.strip()}]'

    # Match anything inside [ ] that contains digits
    new_text = re.sub(r'\[(\d+[^\]]*)\]', replacer, text)

    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(new_text)

clean_refs('docs/academic/ВКР_черновик_claude.md')
print('Removed page numbers from references.')
