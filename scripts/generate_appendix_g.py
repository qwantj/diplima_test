
import re

def format_sql_gost(code):
  # Replace tabs with 2 spaces
  code = code.replace('\t', '  ')
  # Heuristic: convert 4 spaces to 2 spaces
  lines = code.splitlines()
  new_lines = []
  for line in lines:
    if not line.strip():
      new_lines.append('')
      continue
    leading_spaces = len(line) - len(line.lstrip(' '))
    if leading_spaces > 0 and leading_spaces % 4 == 0:
      line = ' ' * (leading_spaces // 2) + line.lstrip(' ')
    new_lines.append(line)
  return '\n'.join(new_lines)

# Full SQL script content for Appendix G
sql_script = """-- ==============================================================================
-- Файл: schema.sql
-- Описание: SQL-скрипт инициализации структуры базы данных PostgreSQL.
-- Обеспечивает хранение сессий, событий классификации и инцидентов атак.
-- Оформлено в соответствии с требованиями ГОСТ 19.402-78.
-- ==============================================================================

-- 1. Таблица сессий мониторинга
-- Хранит метаданные о запусках программного комплекса
CREATE TABLE IF NOT EXISTS sessions (
  id SERIAL PRIMARY KEY,              -- Уникальный идентификатор сессии
  interface_name TEXT,                -- Имя сетевого интерфейса
  model_name TEXT,                    -- Имя использованной ML-модели
  start_time TIMESTAMP DEFAULT NOW(), -- Время запуска мониторинга
  end_time TIMESTAMP,                 -- Время завершения работы
  total_packets BIGINT DEFAULT 0,     -- Всего обработано пакетов
  total_attacks BIGINT DEFAULT 0,     -- Обнаружено вредоносных окон
  total_benign BIGINT DEFAULT 0       -- Обнаружено легитимных окон
);

-- 2. Таблица событий обнаружения
-- Основное хранилище результатов классификации каждого окна агрегации
CREATE TABLE IF NOT EXISTS events (
  id SERIAL PRIMARY KEY,              -- Уникальный идентификатор события
  session_id INT REFERENCES sessions(id), -- Ссылка на родительскую сессию
  timestamp TIMESTAMP DEFAULT NOW(),  -- Метка времени события
  label INT,                          -- Метка класса (0 - норма, 1 - атака)
  confidence REAL,                    -- Уверенность классификатора (0.0 - 1.0)
  pps REAL,                           -- Интенсивность трафика (пак/сек)
  total_packets BIGINT,               -- Общее число пакетов в окне
  features JSONB,                     -- Вектор признаков в формате JSON
  model_name TEXT                     -- Имя модели, выполнившей инференс
);

-- 3. Таблица снимков статистики
-- Используется для построения графиков в реальном времени
CREATE TABLE IF NOT EXISTS stats_snapshots (
  id SERIAL PRIMARY KEY,              -- Уникальный идентификатор снимка
  session_id INT REFERENCES sessions(id), -- Ссылка на сессию
  timestamp TIMESTAMP DEFAULT NOW(),  -- Метка времени снимка
  packets_per_s REAL,                 -- Текущий PPS
  total_packets BIGINT,               -- Нарастающий итог числа пакетов
  current_label INT                   -- Текущее состояние (атака/норма)
);

-- 4. Таблица инцидентов безопасности
-- Агрегированные данные о выявленных атаках для отчетности
CREATE TABLE IF NOT EXISTS security_events (
  id SERIAL PRIMARY KEY,              -- Уникальный идентификатор инцидента
  session_id INT REFERENCES sessions(id), -- Ссылка на сессию
  start_time TIMESTAMP,               -- Момент начала атаки
  duration_sec REAL,                  -- Продолжительность атаки в секундах
  attacker_ip TEXT,                   -- IP-адрес основного источника атаки
  pps_max REAL,                       -- Пиковая интенсивность за время инцидента
  type_label TEXT,                    -- Тип обнаруженной атаки
  confidence REAL                     -- Средняя/максимальная уверенность
);

-- Индексы для оптимизации выборок по сессиям и времени
CREATE INDEX IF NOT EXISTS idx_events_session_id ON events(session_id);
CREATE INDEX IF NOT EXISTS idx_events_timestamp ON events(timestamp);
CREATE INDEX IF NOT EXISTS idx_security_session ON security_events(session_id);
"""

formatted_sql = format_sql_gost(sql_script)

with open('docs/academic/ВКР_черновик_claude.md', 'r', encoding='utf-8') as f:
  vkr = f.read()

# Build Appendix G content
app_g_content = f"""## ПРИЛОЖЕНИЕ Г

Схема базы данных (SQL-скрипт)

### 1. DDL-скрипт инициализации таблиц

```sql
{formatted_sql}
```

### 2. Описание логической структуры

База данных реализована в РСУБД PostgreSQL. Структура спроектирована для обеспечения высокой скорости вставки (до 1000 записей в пакете) и эффективного поиска инцидентов в исторических данных. Связи между таблицами организованы по принципу «один-ко-многим» (One-to-Many) с использованием внешних ключей (Foreign Keys), ссылающихся на таблицу `sessions`.
"""

# Replace placeholder in the document
# pattern matches both ## and ### headers for App G and stops before App D or end of file
pattern = r'#+\s*Приложение Г\..*?ERD-диаграмма\)'
vkr = re.sub(pattern, app_g_content, vkr, flags=re.DOTALL | re.IGNORECASE)

with open('docs/academic/ВКР_черновик_claude.md', 'w', encoding='utf-8') as f:
  f.write(vkr)

print("Appendix G has been generated and injected into the document.")
