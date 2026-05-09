# Устранение неполадок

  

Дата актуализации: 08.04.2026

  

## 1. Проблемы сборки

  

### Qt не найден при конфигурации CMake

  

Симптом:

  

- ошибка find_package(Qt6 ...)

  

Решение:

  

1. Убедитесь, что установлен Qt MSVC kit.

2. Проверьте путь Qt в CMakeLists (CMAKE_PREFIX_PATH).

3. Перегенерируйте build:

  

```cmd

cd c:\Dev\CXX\diploma_test

rmdir /s /q build

mkdir build

cd build

cmake -G "Visual Studio 17 2022" -A x64 ..

```

  

### Конфликт библиотек MinGW/MSVC

  

Симптом:

  

- LNK1107 или смешение ABI

  

Решение:

  

- используйте только MSVC generator;

- удалите старый CMakeCache;

- уберите MinGW пути из PATH.

  

### Не найдены onnxruntime.lib или onnxruntime.dll

  

Решение:

  

- проверьте C:/Lib/onnxruntime/include и C:/Lib/onnxruntime/lib;

- убедитесь, что onnxruntime.dll копируется в build/Release.

  

## 2. Проблемы запуска коллектора

  

### Интерфейс не найден

  

Решение:

  

```cmd

.\ddos_collector.exe --list-interfaces

.\ddos_collector.exe -i "Loopback"

```

  

Запускайте терминал от администратора.

  

### Драйвер Npcap/WinPcap не найден

  

Решение:

  

- переустановите Npcap в режиме WinPcap-compatible;

- проверьте наличие службы NPF.

  

### Порт занят

  

По умолчанию используется порт 50050.

  

```cmd

netstat -ano | findstr :50050

taskkill /PID <PID> /F

```

  

Или запустите collector на другом порту:

  

```cmd

.\ddos_collector.exe -i "WiFi" --tcp-port 50051

```

  

## 3. Проблемы монитора

  

### Монитор не подключается к коллектору

  

Проверьте:

  

- collector действительно запущен;

- порт совпадает (по умолчанию 50050);

- локальный firewall не блокирует порт.

  

### Графики не обновляются

  

Проверьте:

  

- есть ли входящий трафик на выбранном интерфейсе;

- не завис ли collector в режиме replay;

- появляются ли новые stats в консоли collector.

  

## 4. Проблемы базы данных

  

### Ошибка подключения к PostgreSQL

  

Проверьте:

  

- сервис PostgreSQL запущен;

- параметры --db-host/--db-port/--db-name/--db-user/--db-password;

- база ddos_detection_db существует.

  

### Нет данных в истории

  

Проверьте:

  

- подключение к БД в monitor;

- что collector запущен с доступом к БД;

- что отрабатывает асинхронный writer (flush каждые 5 сек).

  

## 5. Проблемы модели (RF/MLP)

  

### RF определяет атаку, а MLP в режиме реального времени работает хуже

  

Частые причины:

  

- сдвиг распределения между train и реальным трафиком;

- несоответствие пары scaler/model;

- перенасыщение признаков при экстремальном PPS;

- слабая калибровка MLP на реальном профиле трафика.

  

Рекомендации:

  

1. Проверить, что загружен корректный файл *_scaler_params.json.

2. Снять и сравнить распределения признаков train vs live.

3. Добавить live-подобные данные в retraining.

4. Проверить use_log1p в scaler-конфиге.

  

## 6. Полезные команды

  

```cmd

# Список интерфейсов

.\ddos_collector.exe --list-interfaces

  

# Захват в режиме реального времени

.\ddos_collector.exe -i "WiFi"

  

# Офлайн-воспроизведение

.\ddos_collector.exe --pcap c:\path\attack.pcap

  

# Смена модели

.\ddos_collector.exe -i "WiFi" --model models\mlp_model.onnx

```