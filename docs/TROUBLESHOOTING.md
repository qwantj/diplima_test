# Устранение неполадок

**Версия:** 2.2  
**Дата актуализации:** 21.05.2026

---

## 1. Проблемы сборки

### Проблема: Qt не найден при конфигурации CMake

**Симптом:**
```
CMake Error: Could not find a package configuration file provided by "Qt6"
```

**Решение:**
1. Убедитесь, что установлен компонент **Qt 6.6+ MSVC 2022 64-bit** в Qt Installer.
2. Укажите путь к Qt при конфигурации CMake:
   ```powershell
   cmake -S . -B build_msvc -G "Visual Studio 17 2022" -A x64 `
     -DCMAKE_PREFIX_PATH="C:/Qt/6.6.0/msvc2019_64"
   ```
3. Или задайте переменную окружения:
   ```powershell
   $env:CMAKE_PREFIX_PATH = "C:\Qt\6.6.0\msvc2019_64"
   ```

---

### Проблема: ONNX Runtime не найден

**Симптом:**
```
CMake Warning: ONNX Runtime not found at C:/Lib/onnxruntime. Building without ML support.
```

**Решение:**
1. Скачайте `onnxruntime-win-x64-<version>.zip` с [GitHub](https://github.com/microsoft/onnxruntime/releases).
2. Распакуйте в `C:\Lib\onnxruntime\`:
   ```
   C:\Lib\onnxruntime\
   ├── include\onnxruntime_cxx_api.h
   └── lib\onnxruntime.lib
       lib\onnxruntime.dll
   ```
3. Если путь другой, укажите его в CMake:
   ```powershell
   cmake ... -DONNXRUNTIME_ROOT="D:/MyLibs/onnxruntime"
   ```

---

### Проблема: Конфликт MinGW/MSVC

**Симптом:**
```
LNK1107: invalid or corrupt file: cannot read at 0x...
```
или смешение ABI.

**Решение:**
1. Используйте **только** генератор `"Visual Studio 17 2022"` — не `"MinGW Makefiles"`.
2. Удалите старый CMakeCache:
   ```powershell
   Remove-Item build_msvc\CMakeCache.txt
   ```
3. Временно уберите MinGW из PATH:
   ```powershell
   $env:PATH = $env:PATH -replace "C:\\msys64\\mingw64\\bin;", ""
   ```
4. Пересоберите с нуля.

---

### Проблема: libpq.dll не найден в Runtime

**Симптом:** Ошибка при запуске — "libpq.dll was not found"

**Решение:**
CMakeLists.txt должен автоматически копировать `libpq.dll`. Если нет:
```powershell
# Найти libpq.dll и скопировать вручную
$vcpkgDir = "C:\vcpkg\installed\x64-windows\bin"
Copy-Item "$vcpkgDir\libpq.dll" ".\build_msvc\Release\"
```

---

### Проблема: Qt DLL не найдены при запуске

**Решение:**
```powershell
cd build_msvc\Release
C:\Qt\6.6.0\msvc2019_64\bin\windeployqt.exe ddos_monitor.exe
C:\Qt\6.6.0\msvc2019_64\bin\windeployqt.exe ddos_collector.exe
```

---

## 2. Проблемы запуска коллектора

### Проблема: Интерфейс не найден

**Симптом:**
```
[error] Failed to initialize detection engine.
```
или `found 0 raw devices`.

**Решение:**
1. **Запускайте от имени администратора** — Npcap требует прав для захвата.
2. Убедитесь, что Npcap установлен в режиме WinPcap-compatible:
   ```powershell
   # Проверить службу Npcap
   Get-Service -Name npcap
   ```
3. Получить список доступных интерфейсов:
   ```powershell
   .\ddos_collector.exe --list-interfaces
   ```
4. Использовать точное имя из списка:
   ```powershell
   .\ddos_collector.exe -i "Беспроводная сеть"
   # или по IP:
   .\ddos_collector.exe -i "192.168.1.5"
   ```

---

### Проблема: Порт 50050 занят

**Симптом:**
```
[error] Failed to start TCP server on 127.0.0.1 port 50050
```

**Решение:**
```powershell
# Найти процесс, занимающий порт
netstat -ano | findstr :50050

# Убить процесс
taskkill /PID <PID> /F

# Или использовать другой порт
.\ddos_collector.exe -i "WiFi" --tcp-port 50051
# В config.json изменить tcp_port на 50051
```

---

### Проблема: Модель не загружается

**Симптом:**
```
[error] Failed to initialize detection engine.
```

**Решение:**
1. Убедитесь, что папка `models/` существует рядом с исполняемым файлом.
2. Проверьте наличие файлов:
   ```powershell
   ls .\models\
   # Должны быть: rf_model.onnx, rf_scaler_params.json
   ```
3. CMake автоматически копирует `models/` в директорию сборки. Если не скопировалось:
   ```powershell
   Copy-Item -Recurse .\models\ ".\build_msvc\Release\models\"
   ```
4. Явно указать путь к модели:
   ```powershell
   .\ddos_collector.exe -i "WiFi" --model "C:\full\path\to\rf_model.onnx"
   ```

---

## 3. Проблемы монитора

### Проблема: Монитор не подключается к коллектору

**Симптом:** Индикатор "Live" не показывает подключения.

**Решение:**
1. Убедитесь, что коллектор запущен и нет ошибок в его логе:
   ```powershell
   Get-Content ddos_collector.log -Tail 20
   ```
2. Проверьте, что порт в `config.json` совпадает у обоих приложений (по умолчанию 50050).
3. Проверьте Windows Firewall — не блокирует ли порт:
   ```powershell
   netsh advfirewall firewall add rule name="DDoS Monitor" `
     dir=in action=allow protocol=TCP localport=50050
   ```
4. Проверьте подключение:
   ```powershell
   Test-NetConnection -ComputerName 127.0.0.1 -Port 50050
   ```

---

### Проблема: Графики не обновляются

**Причины и решения:**
- Нет трафика на интерфейсе → сгенерируйте трафик (ping, браузер).
- Коллектор завис в режиме replay → нажмите кнопку **⏹ Live** в GUI.
- Слишком мало трафика (< 50 PPS) → срабатывает noise threshold, это норма.
- Проверьте лог коллектора на наличие stats-сообщений:
  ```powershell
  Get-Content ddos_collector.log | Select-String "pps"
  ```

---

### Проблема: DB: Disconnected в тулбаре

**Симптом:** Статус БД красный, история не загружается.

**Решение:**
1. Убедитесь, что PostgreSQL запущен:
   ```powershell
   Get-Service -Name postgresql*
   ```
2. Проверьте параметры подключения в `config.json`.
3. Проверьте существование базы данных:
   ```powershell
   psql -U postgres -c "\l" | findstr ddos_detection_db
   # Если не существует:
   psql -U postgres -c "CREATE DATABASE ddos_detection_db;"
   ```
4. Проверьте лог монитора:
   ```powershell
   Get-Content ddos_monitor.log | Select-String "Database"
   ```

---

## 4. Проблемы базы данных

### Проблема: Нет данных в истории сессий

**Решение:**
1. Убедитесь, что коллектор подключился к БД (в логе должно быть "Successfully connected").
2. Данные записываются асинхронно с задержкой до 5 сек — подождите.
3. Проверьте таблицы напрямую:
   ```sql
   SELECT * FROM sessions ORDER BY id DESC LIMIT 5;
   SELECT COUNT(*) FROM events;
   ```

---

### Проблема: Ошибки pqxx при записи

**Симптом:** В логе `[error] DatabaseManager: events batch failed: ...`

**Частые причины:**
- Таблицы не созданы → перезапустите коллектор (таблицы создаются при старте).
- Переполнение диска → проверьте свободное место.
- Недостаточно прав у пользователя PostgreSQL:
  ```sql
  GRANT ALL PRIVILEGES ON DATABASE ddos_detection_db TO postgres;
  ```

---

## 5. Проблемы ML-моделей

### Проблема: Всё классифицируется как атака

**Причины:**
- Несоответствие пары model/scaler → убедитесь, что `rf_scaler_params.json` используется с `rf_model.onnx`.
- Сдвиг распределения → попробуйте переключиться на другую модель в GUI.
- Слишком высокий фоновый трафик.

**Диагностика:**
1. Включить вывод признаков в лог (временно в `DetectionEngine.cpp`):
   ```cpp
   AppLogger::get()->debug("Features: {}", nlohmann::json(features).dump());
   ```
2. Сравнить значения с `mean` из scaler_params.json — должны быть близки.

---

### Проблема: RF работает, а MLP не детектирует атаки

**Причины:**
- MLP более чувствителен к дрейфу распределений.
- Неправильный скейлер (MLP требует `mlp_scaler_params.json`).

**Решение:**
1. Переключиться на RF в GUI (выбрать `rf_model.onnx` в комбобоксе).
2. Для MLP включить `"use_log1p": true` в `mlp_scaler_params.json`.

---

## 6. Полезные команды

```powershell
# === Коллектор ===

# Список интерфейсов
.\ddos_collector.exe --list-interfaces

# Захват с интерфейса WiFi
.\ddos_collector.exe -i "WiFi"

# Захват с MLP моделью
.\ddos_collector.exe -i "WiFi" --model models\mlp_model.onnx

# Анализ PCAP-файла
.\ddos_collector.exe --pcap C:\attacks\ddos_sample.pcap

# Захват с записью дампов
.\ddos_collector.exe -i "WiFi" --pcap-dir pcap_dumps

# Нестандартный порт и БД
.\ddos_collector.exe -i "WiFi" --tcp-port 50051 `
  --db-host localhost --db-name ddos_detection_db `
  --db-user postgres --db-password secret

# === Мониторинг ===

# Просмотр лога коллектора в реальном времени
Get-Content ddos_collector.log -Tail 50 -Wait

# Просмотр лога монитора
Get-Content ddos_monitor.log -Tail 50 -Wait

# === База данных ===

# Подключиться к PostgreSQL
psql -U postgres -d ddos_detection_db

# Последние 10 сессий
psql -U postgres -d ddos_detection_db -c "SELECT id, interface_name, total_packets, total_attacks FROM sessions ORDER BY id DESC LIMIT 10;"

# Последние инциденты
psql -U postgres -d ddos_detection_db -c "SELECT start_time, duration_sec, attacker_ip, pps_max FROM security_events ORDER BY start_time DESC LIMIT 10;"

# === Диагностика сети ===

# Проверить порт коллектора
netstat -ano | findstr :50050

# Проверить доступность порта
Test-NetConnection -ComputerName 127.0.0.1 -Port 50050

# Убить процесс на порту
netstat -ano | findstr :50050
taskkill /PID <PID> /F
```

---

## 7. Часто задаваемые вопросы (FAQ)

**Q: Можно ли запускать collector и monitor на разных компьютерах?**  
A: Да. В `config.json` монитора установите `collector_host` на IP коллектора. Убедитесь, что порт 50050 открыт в firewall.

**Q: Можно ли запустить без PostgreSQL?**  
A: Да. Коллектор продолжает работу без БД — детекция и TCP-трансляция работают. История сессий в мониторе будет недоступна.

**Q: Как добавить новую сеть в список интерфейсов?**  
A: Используйте `--list-interfaces` для получения точного имени интерфейса системы.

**Q: Почему коллектор не детектирует трафик при < 50 PPS?**  
A: Это намеренная защита от шума. При очень малом трафике ML-признаки ненадёжны. Порог `NOISE_THRESHOLD_PPS = 50.0` задан в `DetectionEngine.hpp`.

**Q: Как использовать GPU для инференса?**  
A: Установите CUDA Toolkit и запустите коллектор с `--ep cuda`. Для DirectML (без NVIDIA): `--ep dml`.

**Q: Где хранятся PCAP-дампы?**  
A: В папке `pcap_dumps/` рядом с исполняемым файлом (или в директории, указанной через `--pcap-dir`).