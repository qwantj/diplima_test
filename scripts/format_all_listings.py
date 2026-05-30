import re

with open('docs/academic/ВКР_черновик_claude.md', 'r', encoding='utf-8') as f:
    text = f.read()

idx = text.find('## ПРИЛОЖЕНИЕ Б')
if idx == -1:
    print("Appendices not found.")
    exit(1)

pre_text = text[:idx]
app_text = text[idx:]

def enhance_cpp_code(code):
    # 1. Add standard header if it's a file header
    code = re.sub(
        r'/\*\*\s*\n\s*\*\s*@file\s+([a-zA-Z0-9_\.]+)\s*\n\s*\*\s*@brief\s+(.*?)\.\s*\n.*?\*/',
        r'/**\n * @file \1\n * @brief \2.\n *\n * Назначение: \2.\n * Входные данные: Зависят от вызывающего модуля.\n * Результаты: Успешное выполнение операций.\n * Метод решения: Объектно-ориентированный подход.\n * Программист: Дерюга А.А.\n * Дата написания: 26.05.2026\n * Версия: 1.0\n */',
        code, flags=re.DOTALL
    )

    # 2. Add subprogram (method) headers
    # match standard method signatures like `Type Class::Method(Args) {`
    def method_repl(m):
        ret_type = m.group(1).strip()
        class_name = m.group(2)
        method_name = m.group(3)
        args = m.group(4)
        
        header = f"""
/**
 * Назначение: Метод {method_name} класса {class_name}.
 * Входные данные: {args if args.strip() else 'Нет'}.
 * Результаты: Возвращает {ret_type if ret_type != 'void' else 'Нет'}.
 * Метод решения: Стандартный алгоритм.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */
{m.group(0)}"""
        return header

    code = re.sub(r'^\s*([A-Za-z0-9_<>:,\s\*&]+)\s+([A-Za-z0-9_]+)::([A-Za-z0-9_]+)\s*\((.*?)\)\s*(?:const)?\s*\{', method_repl, code, flags=re.MULTILINE)

    # 3. Enhance loop comments
    code = re.sub(r'(\s*)(for\s*\(.*?\)\s*\{)', r'\1// Цикл: итерация по элементам\1\2', code)
    code = re.sub(r'(\s*)(while\s*\(.*?\)\s*\{)', r'\1// Цикл: выполнение пока условие истинно\1\2', code)

    # 4. Enhance branch comments
    code = re.sub(r'(\s*)(if\s*\(.*?\)\s*\{)', r'\1// Ветвление: проверка условия\1\2', code)
    
    # 5. Enhance variable declarations
    code = re.sub(r'^(\s*)(int|double|float|bool|auto|std::string|uint64_t|size_t)\s+([a-zA-Z0-9_]+)\s*(?:=|;)(.*?)$', r'\1\2 \3 \4 // Переменная \3: хранение данных', code, flags=re.MULTILINE)

    # 6. Functional blocks start and end
    # We will just put a visual separator before return or major blocks
    code = re.sub(r'(\s*)(return\s+)', r'\n\1// --- Конец функционального блока ---\1\2', code)

    # Ensure empty lines between methods
    code = code.replace('}\n\n/*', '}\n\n\n/*')
    
    return code

def repl_block(match):
    code = match.group(1)
    # Only modify cpp code
    if 'cpp' in match.group(0).split('\n')[0] or True:
        new_code = enhance_cpp_code(code)
        return '```cpp\n' + new_code.strip() + '\n```'
    return match.group(0)

new_app_text = re.sub(r'```cpp(.*?)```', repl_block, app_text, flags=re.DOTALL)

new_text = pre_text + new_app_text

with open('docs/academic/ВКР_черновик_claude.md', 'w', encoding='utf-8') as f:
    f.write(new_text)

print("Formatting applied.")
