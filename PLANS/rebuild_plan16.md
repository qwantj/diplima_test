# План очистки и повторной сборки проекта

Этот план описывает шаги для полной очистки предыдущих артефактов сборки и выполнения свежей компиляции проекта с использованием компилятора MSVC.

## Proposed Changes

### Build System

#### [NEW] build_msvc_3 (Directory)
Будет создана новая директория для сборки, чтобы обойти блокировки файлов в старой директории `build_msvc`.

## Verification Plan

### Automated Tests
1. **Конфигурация**: `cmake -G "Visual Studio 17 2022" -A x64 -S . -B build_msvc_3`
   - Ожидаемый результат: Успешное создание файлов проекта.
2. **Сборка**: `cmake --build build_msvc_3 --config Release --parallel`
   - Ожидаемый результат: Компиляция всех целей (`ddos_core`, `ddos_collector`, `ddos_monitor`) без ошибок.
3. **Проверка артефактов**: Проверка наличия исполняемых файлов и скопированных DLL в `build_msvc_3/Release`.

### Manual Verification
1. Запуск `ddos_collector.exe` и `ddos_monitor.exe` из папки сборки для подтверждения работоспособности.
