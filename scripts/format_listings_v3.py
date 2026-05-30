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
    # 1. File header
    code = re.sub(
        r'/\*\*\s*\n\s*\*\s*@file\s+([a-zA-Z0-9_\.]+)\s*\n\s*\*\s*@brief\s+(.*?)\.\s*\n.*?\*/',
        r'/**\n * Название: \1\n * Назначение: \2.\n * Входные данные: зависят от контекста.\n * Результаты: инициализация модуля.\n * Метод решения: структурный подход.\n * Программист: Дерюга А.А.\n * Дата написания: 26.05.2026\n * Версия: 1.0\n */',
        code, flags=re.DOTALL
    )

    # 2. Add subprogram (method) headers
    def method_repl(m):
        ret_type = m.group(1).strip()
        class_name = m.group(2)
        method_name = m.group(3)
        args = m.group(4)
        rest = m.group(5)
        
        # Avoid duplicate headers
        if 'Назначение:' in ret_type:
            return m.group(0)

        header = f"""
// Название: {class_name}::{method_name}
// Назначение: Выполнение основной логики метода {method_name}
// Входные данные: {args if args.strip() else 'отсутствуют'}
// Результаты: {ret_type if ret_type != 'void' else 'изменение состояния объекта'}
// Метод решения: императивный подход
// Программист: Дерюга А.А.
// Дата написания: 26.05.2026
// Версия: 1.0
{m.group(0).lstrip()}"""
        return '\n\n' + header

    code = re.sub(r'\n([a-zA-Z0-9_<>:\*&\s]+)\s+([a-zA-Z0-9_]+)::([a-zA-Z0-9_]+)\s*\((.*?)\)(.*?)\{', method_repl, code)

    # 3. Enhance loop comments
    code = re.sub(r'(\n\s*)(for\s*\(.*?\)\s*\{)', r'\1// Цикл: перебор элементов\1\2', code)
    code = re.sub(r'(\n\s*)(while\s*\(.*?\)\s*\{)', r'\1// Цикл: итеративное выполнение до выполнения условия\1\2', code)

    # 4. Enhance branch comments
    code = re.sub(r'(\n\s*)(if\s*\(.*?\)\s*\{)', r'\1// Ветвление: проверка логического условия\1\2', code)
    code = re.sub(r'(\n\s*)(else\s*\{)', r'\1// Альтернативная ветвь обработки\1\2', code)
    code = re.sub(r'(\n\s*)(else\s+if\s*\(.*?\)\s*\{)', r'\1// Альтернативная ветвь с условием\1\2', code)

    # 5. Enhance variable declarations
    def var_repl(m):
        var_type = m.group(2)
        var_name = m.group(3)
        if var_type == 'return' or var_type == 'else':
            return m.group(0)
        return f"{m.group(0)} // Назначение: локальная переменная {var_name}"

    code = re.sub(r'(\n\s*)([a-zA-Z0-9_<>:]+)\s+([a-zA-Z0-9_]+)\s*=\s*(.*?);', var_repl, code)
    
    def var_repl2(m):
        var_type = m.group(2)
        var_name = m.group(3)
        if var_type == 'return' or var_type == 'else':
            return m.group(0)
        return f"{m.group(0)} // Назначение: локальная переменная {var_name}"
    
    code = re.sub(r'(\n\s*)(int|double|float|bool|auto|std::string|uint64_t|size_t)\s+([a-zA-Z0-9_]+)\s*;', var_repl2, code)

    # 6. Functional blocks start and end
    code = re.sub(r'(\n\s*)(return\s+.*?;)', r'\n\1// --- Конец функциональной части ---\1\2', code)
    code = re.sub(r'(\)\s*(?:const)?\s*\{\n)', r'\1    // --- Начало функциональной части ---\n', code)

    # Clean up double injections
    code = code.replace('// Назначение: локальная переменная // Назначение', '// Назначение')
    
    return code

def repl_block(match):
    code = match.group(1)
    if 'cpp' in match.group(0).split('\n')[0]:
        new_code = enhance_cpp_code(code)
        return '```cpp\n' + new_code.strip() + '\n```'
    return match.group(0)

new_app_text = re.sub(r'```cpp(.*?)```', repl_block, app_text, flags=re.DOTALL)
new_text = pre_text + new_app_text

with open('docs/academic/ВКР_черновик_claude.md', 'w', encoding='utf-8') as f:
    f.write(new_text)

print("Formatting applied safely.")
