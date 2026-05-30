import re

def process_file():
    filepath = 'docs/academic/ВКР_черновик_claude.md'
    with open(filepath, 'r', encoding='utf-8') as f:
        text = f.read()

    # Split into Appendix B and V
    app_b_start = text.find('## ПРИЛОЖЕНИЕ Б')
    app_v_start = text.find('## ПРИЛОЖЕНИЕ В')
    app_g_start = text.find('## ПРИЛОЖЕНИЕ Г')

    if app_b_start == -1 or app_v_start == -1 or app_g_start == -1:
        print("Could not find appendices")
        return

    part1 = text[:app_b_start]
    app_b = text[app_b_start:app_v_start]
    app_v = text[app_v_start:app_g_start]
    part_end = text[app_g_start:]

    def clean_appendix(app_text, keep_cpp_only=False):
        pattern = r'(### [^\n]+?\(([^)]+\.(?:cpp|hpp|h|c))\)\n*```cpp\n.*?\n```)'
        
        def replacer(match):
            full_block = match.group(1)
            filename = match.group(2)
            
            # If it's a header file, remove entirely
            if filename.endswith('.hpp') or filename.endswith('.h'):
                return ''
                
            # If we need to keep specific cpp files
            if keep_cpp_only:
                if filename not in ['TcpClient.cpp', 'DataBridge.cpp']:
                    return ''
                    
            # Clean C++ code
            code_start = full_block.find('```cpp\n') + 7
            code_end = full_block.rfind('\n```')
            code = full_block[code_start:code_end]
            
            # Remove includes
            code = re.sub(r'#include\s*[<"][^>"]+[>"]\n*', '', code)
            code = re.sub(r'#pragma\s+once\n*', '', code)
            
            # Remove the top Doxygen comment if it exists
            code = re.sub(r'/\*\*.*?\*/\n*', '', code, flags=re.DOTALL)
            
            # Remove method Doxygen comments
            code = re.sub(r'// Назначение:.*?\n', '', code)
            code = re.sub(r'// Входные данные:.*?\n', '', code)
            code = re.sub(r'// Результаты:.*?\n', '', code)
            code = re.sub(r'// Метод решения:.*?\n', '', code)
            code = re.sub(r'// Программист:.*?\n', '', code)
            code = re.sub(r'// Дата написания:.*?\n', '', code)
            code = re.sub(r'// Версия:.*?\n', '', code)
            code = re.sub(r'// --- Начало функционально законченной части ---\n*', '', code)
            code = re.sub(r'// --- Конец функционально законченной части ---\n*', '', code)
            
            # Remove empty destructors
            code = re.sub(r'[A-Za-z0-9_]+::~[A-Za-z0-9_]+\(\)\s*\{\s*\}\n*', '', code)
            
            # Clean up empty lines
            code = re.sub(r'\n{3,}', '\n\n', code)
            
            header = """/*
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */\n\n"""
            
            new_block = full_block[:code_start] + header + code.strip() + '\n' + full_block[code_end:]
            return new_block
            
        new_app_text = re.sub(pattern, replacer, app_text, flags=re.DOTALL)
        # Remove any empty leftover lines where headers used to be
        new_app_text = re.sub(r'\n{3,}', '\n\n', new_app_text)
        return new_app_text

    app_b = clean_appendix(app_b, keep_cpp_only=False)
    app_v = clean_appendix(app_v, keep_cpp_only=True)

    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(part1 + app_b + app_v + part_end)
    print("Done")

if __name__ == '__main__':
    process_file()