# Windows Setup Guide (MSVC + Qt 6 + vcpkg)

Эта инструкция предназначена для настройки окружения разработки под Windows 10/11 с использованием компилятора MSVC.

## 1. Предварительные требования

Установите:
- **Visual Studio 2022** (Community/Professional/Enterprise).
  - Обязательный компонент: **Разработка классических приложений на C++** (Desktop development with C++).
- **Git for Windows**.
- **CMake 3.24+** (можно использовать встроенный в Visual Studio или установить отдельно).

## 2. Установка Qt 6

1. Скачайте **Qt Online Installer**: <https://www.qt.io/download>
2. В установщике выберите:
   - Версию **Qt 6.6+** (например, 6.6.2 или 6.10.x).
   - Компонент **MSVC 2022 64-bit**.
3. Установите переменные окружения:
   - `QTDIR` = `C:\Qt\6.6.2\msvc2022_64` (или ваш путь).
   - `Qt6_DIR` = `%QTDIR%\lib\cmake\Qt6`.
   - Добавьте `%QTDIR%\bin` в переменную `PATH`.

## 3. Установка vcpkg и зависимостей

1. Клонируйте vcpkg (рекомендуется в корень диска, например `C:\vcpkg`):
```cmd
git clone https://github.com/microsoft/vcpkg C:\vcpkg
cd C:\vcpkg
bootstrap-vcpkg.bat
```
2. Добавьте системную переменную среды `VCPKG_ROOT` = `C:\vcpkg`.
3. Установите необходимые библиотеки:
```cmd
%VCPKG_ROOT%\vcpkg install pcapplusplus:x64-windows
%VCPKG_ROOT%\vcpkg install onnxruntime:x64-windows
```

## 4. Конфигурация CMake

В корне проекта выполните:
```cmd
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
  -DCMAKE_PREFIX_PATH=%QTDIR% ^
  -DQt6_DIR=%QTDIR%\lib\cmake\Qt6
```

Сборка проекта:
```cmd
cmake --build build --config Release
```

## 5. Возможные проблемы сборки (Troubleshooting)

- **Qt не находится (Could NOT find Qt6):** 
  Проверьте, что переменная `Qt6_DIR` указывает именно на папку `lib\cmake\Qt6` внутри установленного Qt для MSVC.
- **Ошибки vcpkg (не найдены пакеты):**
  Убедитесь, что вы передаете `-DCMAKE_TOOLCHAIN_FILE` при первой конфигурации CMake. Если вы забыли это сделать, удалите папку `build` и запустите CMake заново.
- **PcapPlusPlus требует WinPcap/Npcap SDK:**
  Пакет `pcapplusplus[pcap]` в vcpkg автоматически скачивает необходимые заголовки, но для *запуска* скомпилированного приложения у вас должен быть установлен драйвер **Npcap** (в режиме совместимости с WinPcap) на вашей системе.
