import re

with open('docs/academic/ВКР_черновик_claude.md', 'r', encoding='utf-8') as f:
    text = f.read()

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
            new_lines.append(' * Входные данные: Зависят от контекста вызова')
            new_lines.append(' * Результаты: Успешное выполнение операций')
            new_lines.append(' * Метод решения: Объектно-ориентированный подход')
            new_lines.append(' * Программист: Дерюга А.А.')
            new_lines.append(' * Дата написания: 26.05.2026')
            new_lines.append(' * Версия: 1.0')
            continue
            
        if stripped.startswith('* @brief') or stripped.startswith('* Разработано') or stripped.startswith('* Назначение:') or stripped.startswith('* Входные данные:') or stripped.startswith('* Результаты:') or stripped.startswith('* Метод решения:') or stripped.startswith('* Программист:') or stripped.startswith('* Дата написания:') or stripped.startswith('* Версия:'):
            continue # Skip old brief to not duplicate
            
        # 2. Method definitions
        # Allow commas in return type
        method_match = re.match(r'^([a-zA-Z0-9_<>:\*&,\s]*?)\s*([a-zA-Z0-9_]+)::([a-zA-Z0-9_~]+)\s*\((.*?)\)(.*?)\{', line)
        
        if method_match and not ';' in line:
            class_name = method_match.group(2)
            method_name = method_match.group(3)
            args = method_match.group(4)
            if len(new_lines) > 0 and not new_lines[-1].strip() == '':
                new_lines.append('')
            new_lines.append(f'// Назначение: Выполнение логики метода {method_name} класса {class_name}')
            new_lines.append(f'// Входные данные: {args if args.strip() else "Нет"}')
            new_lines.append(f'// Результаты: Изменение состояния объекта или возврат значения')
            new_lines.append(f'// Метод решения: Императивный алгоритм')
            new_lines.append(f'// Программист: Дерюга А.А.')
            new_lines.append(f'// Дата написания: 26.05.2026')
            new_lines.append(f'// Версия: 1.0')
            new_lines.append(line)
            new_lines.append('    // --- Начало функционально законченной части ---')
            in_method = True
            continue
            
        # 3 & 4. Loops and branches
        if stripped.startswith('for ') or stripped.startswith('for('):
            new_lines.append(line.replace('for', '// Цикл: итерация по элементам\n' + line[:len(line)-len(line.lstrip())] + 'for', 1))
            continue
            
        if stripped.startswith('while ') or stripped.startswith('while('):
            new_lines.append(line.replace('while', '// Цикл: итеративное выполнение пока условие истинно\n' + line[:len(line)-len(line.lstrip())] + 'while', 1))
            continue
            
        if stripped.startswith('if ') or stripped.startswith('if('):
            new_lines.append(line.replace('if', '// Ветвление: проверка логического условия\n' + line[:len(line)-len(line.lstrip())] + 'if', 1))
            continue
            
        if stripped.startswith('else ') or stripped == 'else':
            new_lines.append(line.replace('else', '// Альтернативная ветвь обработки\n' + line[:len(line)-len(line.lstrip())] + 'else', 1))
            continue

        # 5. End of functional part (before return)
        if stripped.startswith('return '):
            new_lines.append(line.replace('return', '// --- Конец функционально законченной части ---\n' + line[:len(line)-len(line.lstrip())] + 'return', 1))
            if in_method and stripped.endswith(';'):
                # it's possible the method ends right after return
                pass
            continue
            
        # End of method tracking
        if stripped == '}' and in_method:
            in_method = False
            new_lines.append(line)
            new_lines.append('') # Empty line between subprograms
            continue

        # Data declarations (heuristic)
        var_match = re.match(r'^(\s*)(int|double|float|bool|auto|std::string|uint64_t|size_t)\s+([a-zA-Z0-9_]+)\s*(=|;)(.*?)$', line)
        if var_match and in_method and 'return' not in line:
            var_name = var_match.group(3)
            if '//' not in line:
                new_lines.append(f'{line} // Назначение: локальная переменная {var_name}')
            elif 'Назначение:' not in line:
                # Append to existing comment
                new_lines.append(f'{line} (локальная переменная {var_name})')
            else:
                new_lines.append(line)
            continue

        new_lines.append(line)

    return '\n'.join(new_lines)

def repl_block(match):
    code = match.group(1)
    if 'cpp' in match.group(0).split('\n')[0]:
        new_code = enhance_cpp_code(code)
        return '```cpp\n' + new_code.strip() + '\n```'
    return match.group(0)

new_text = re.sub(r'```cpp(.*?)```', repl_block, text, flags=re.DOTALL)

# fix multiple newlines
new_text = re.sub(r'\n{4,}', '\n\n\n', new_text)

with open('docs/academic/ВКР_черновик_claude.md', 'w', encoding='utf-8') as f:
    f.write(new_text)

print("Formatting applied smoothly v6.")
