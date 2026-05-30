import re

def process_file():
    filepath = 'docs/academic/ВКР_черновик_claude.md'
    with open(filepath, 'r', encoding='utf-8') as f:
        text = f.read()

    # Split into Appendix B and V
    app_b_start = text.find('## ПРИЛОЖЕНИЕ Б')
    app_g_start = text.find('## ПРИЛОЖЕНИЕ Г')

    if app_b_start == -1 or app_g_start == -1:
        print("Could not find appendices")
        return

    part1 = text[:app_b_start]
    apps = text[app_b_start:app_g_start]
    part_end = text[app_g_start:]

    def clean_empty_lines(app_text):
        pattern = r'(```cpp\n)(.*?)(\n```)'
        
        def replacer(match):
            prefix = match.group(1)
            code = match.group(2)
            suffix = match.group(3)
            
            # Replace 2 or more consecutive newline characters with exactly 2 newlines (1 empty line)
            code = re.sub(r'\n{3,}', '\n\n', code)
            
            return prefix + code + suffix
            
        return re.sub(pattern, replacer, app_text, flags=re.DOTALL)

    apps = clean_empty_lines(apps)

    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(part1 + apps + part_end)
    print("Done")

if __name__ == '__main__':
    process_file()