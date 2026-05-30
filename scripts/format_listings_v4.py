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
    lines = code.split('\n')
    new_lines = []
    
    in_method = False
    
    for i, line in enumerate(lines):
        stripped = line.strip()
        
        # 1. File header replacement (only if we find @file)
        if stripped.startswith('* @file'):
            new_lines.append(line)
            filename = stripped.split()[-1]
            new_lines.append(f' * Назначение: Модуль {filename}')
            new_lines.append(' * Входные данные: Зависят от контекста')
            new_lines.append(' * Результаты: Успешная инициализация')
            new_lines.append(' * Метод решения: ООП')
            new_lines.append(' * Программист: Дерюга А.А.')
            new_lines.append(' * Дата написания: 26.05.2026')
            new_lines.append(' * Версия: 1.0')
            continue
            
        if stripped.startswith('* @brief') or stripped.startswith('* Разработано'):
            continue # Skip old brief to not duplicate
            
        # 2. Method definitions
        # Look for something like Type Class::Method(Args) {
        # or Class::Class() {
        method_match = re.match(r'^([a-zA-Z0-9_<>:\*&\s]*?)\s*([a-zA-Z0-9_]+)::([a-zA-Z0-9_~]+)\s*\((.*?)\)(.*?)\{', line)
        if method_match and not ';' in line:
            class_name = method_match.group(2)
            method_name = method_match.group(3)
            args = method_match.group(4)
            if not new_lines[-1].strip() == '':
                new_lines.append('')
            new_lines.append(f'// Назначение: Метод {method_name} класса {class_name}')
            new_lines.append(f'// Входные данные: {args if args.strip() else "Нет"}')
            new_lines.append(f'// Результаты: Выполнение операции')
            new_lines.append(f'// Метод решения: Стандартный алгоритм')
            new_lines.append(f'// Программист: Дерюга А.А.')
            new_lines.append(f'// Дата написания: 26.05.2026')
            new_lines.append(f'// Версия: 1.0')
            new_lines.append(line)
            new_lines.append('    // --- Начало функциональной части ---')
            in_method = True
            continue
            
        # 3 & 4. Loops and branches
        if stripped.startswith('for ') or stripped.startswith('for('):
            new_lines.append(line.replace('for', '// Цикл: итерация по элементам\n' + line[:len(line)-len(line.lstrip())] + 'for', 1))
            continue
            
        if stripped.startswith('while ') or stripped.startswith('while('):
            new_lines.append(line.replace('while', '// Цикл: выполнение пока условие истинно\n' + line[:len(line)-len(line.lstrip())] + 'while', 1))
            continue
            
        if stripped.startswith('if ') or stripped.startswith('if('):
            new_lines.append(line.replace('if', '// Ветвление: проверка логического условия\n' + line[:len(line)-len(line.lstrip())] + 'if', 1))
            continue
            
        # 5. End of functional part (before return)
        if stripped.startswith('return ') and in_method:
            new_lines.append(line.replace('return', '// --- Конец функциональной части ---\n' + line[:len(line)-len(line.lstrip())] + 'return', 1))
            continue
            
        # End of method tracking
        if stripped == '}' and in_method:
            in_method = False
            new_lines.append(line)
            new_lines.append('') # Empty line between subprograms
            continue

        # Data declarations (heuristic)
        # matches: Type name = value; or Type name;
        var_match = re.match(r'^(\s*)(int|double|float|bool|auto|std::string|uint64_t|size_t)\s+([a-zA-Z0-9_]+)\s*(=|;)(.*?)$', line)
        if var_match and in_method and 'return' not in line:
            var_name = var_match.group(3)
            # Add comment at the end of the line
            if '//' not in line:
                new_lines.append(f'{line} // Назначение: переменная {var_name}')
            else:
                new_lines.append(line)
            continue

        new_lines.append(line)

    return '\n'.join(new_lines)

def repl_block(match):
    code = match.group(1)
    if 'cpp' in match.group(0).split('\n')[0] or 'JSON' in match.group(0).split('\n')[0]:
        if 'cpp' in match.group(0).split('\n')[0]:
            new_code = enhance_cpp_code(code)
            return '```cpp\n' + new_code.strip() + '\n```'
    return match.group(0)

new_app_text = re.sub(r'```cpp(.*?)```', repl_block, app_text, flags=re.DOTALL)
new_text = pre_text + new_app_text

# fix multiple newlines
new_text = re.sub(r'\n{4,}', '\n\n\n', new_text)

with open('docs/academic/ВКР_черновик_claude.md', 'w', encoding='utf-8') as f:
    f.write(new_text)

print("Formatting applied smoothly.")
