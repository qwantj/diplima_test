import re

with open('docs/academic/ВКР_черновик_claude.md', 'r', encoding='utf-8') as f:
    text = f.read()

t28_old = """| Поле | Тип PostgreSQL | Описание |---|---|---|---|
|---|---|---|---|
| `id` | `SERIAL PRIMARY KEY` | Уникальный идентификатор | — | `session_id` | `INTEGER` | Внешний ключ к таблице `sessions` | — | `timestamp` | `TIMESTAMP NOT NULL` | Временна́я метка события | — | `label` | `INTEGER NOT NULL` | Метка (0 — норма, 1 — атака) | — | `confidence` | `DOUBLE PRECISION` | Уверенность классификатора [0..1] | — | `PPS` | `DOUBLE PRECISION` | Пакетов в секунду за окно | — | `total_packets` | `BIGINT` | Всего пакетов в окне | — | `features` | `JSONB` | Вектор извлеченных признаков в формате JSON | — | `model_name` | `TEXT` | Имя использованной модели |"""

t28_new = """| Поле | Тип PostgreSQL | Описание |
|---|---|---|
| `id` | `SERIAL PRIMARY KEY` | Уникальный идентификатор |
| `session_id` | `INTEGER` | Внешний ключ к таблице `sessions` |
| `timestamp` | `TIMESTAMP NOT NULL` | Временна́я метка события |
| `label` | `INTEGER NOT NULL` | Метка (0 — норма, 1 — атака) |
| `confidence` | `DOUBLE PRECISION` | Уверенность классификатора [0..1] |
| `PPS` | `DOUBLE PRECISION` | Пакетов в секунду за окно |
| `total_packets` | `BIGINT` | Всего пакетов в окне |
| `features` | `JSONB` | Вектор извлеченных признаков в формате JSON |
| `model_name` | `TEXT` | Имя использованной модели |"""

t29_old = """| Поле | Тип PostgreSQL | Описание |---|---|---|---|
|---|---|---|---|
| `id` | `SERIAL PRIMARY KEY` | Уникальный идентификатор сессии | — | `interface_name` | `TEXT` | Имя сетевого интерфейса | — | `model_name` | `TEXT` | Имя загруженной модели | — | `start_time` | `TIMESTAMP` | Начало сессии | — | `end_time` | `TIMESTAMP` | Конец сессии | — | `total_packets` | `BIGINT` | Всего обработано пакетов | — | `total_attacks` | `BIGINT` | Обнаружено атак (окон с меткой 1) | — | `total_benign` | `BIGINT` | Количество легитимных окон |"""

t29_new = """| Поле | Тип PostgreSQL | Описание |
|---|---|---|
| `id` | `SERIAL PRIMARY KEY` | Уникальный идентификатор сессии |
| `interface_name` | `TEXT` | Имя сетевого интерфейса |
| `model_name` | `TEXT` | Имя загруженной модели |
| `start_time` | `TIMESTAMP` | Начало сессии |
| `end_time` | `TIMESTAMP` | Конец сессии |
| `total_packets` | `BIGINT` | Всего обработано пакетов |
| `total_attacks` | `BIGINT` | Обнаружено атак (окон с меткой 1) |
| `total_benign` | `BIGINT` | Количество легитимных окон |"""

t210_old = """| Поле | Тип PostgreSQL | Описание |---|---|---|---|
|---|---|---|---|
| `id` | `SERIAL PRIMARY KEY` | Уникальный идентификатор инцидента | — | `session_id` | `INTEGER` | Внешний ключ к сессии | — | `start_time` | `TIMESTAMP NOT NULL` | Начало атаки | — | `duration_sec` | `REAL` | Продолжительность атаки (секунды) | — | `attacker_ip` | `TEXT` | IP-адрес основного источника | — | `pps_max` | `REAL` | Пиковая интенсивность (пак/с) | — | `type_label` | `TEXT` | Тип атаки («DDoS Attack» и др.) | — | `confidence` | `REAL` | Максимальная уверенность за инцидент |"""

t210_new = """| Поле | Тип PostgreSQL | Описание |
|---|---|---|
| `id` | `SERIAL PRIMARY KEY` | Уникальный идентификатор инцидента |
| `session_id` | `INTEGER` | Внешний ключ к сессии |
| `start_time` | `TIMESTAMP NOT NULL` | Начало атаки |
| `duration_sec` | `REAL` | Продолжительность атаки (секунды) |
| `attacker_ip` | `TEXT` | IP-адрес основного источника |
| `pps_max` | `REAL` | Пиковая интенсивность (пак/с) |
| `type_label` | `TEXT` | Тип атаки («DDoS Attack» и др.) |
| `confidence` | `REAL` | Максимальная уверенность за инцидент |"""

t211_old = """| Поле | Тип PostgreSQL | Описание |---|---|---|---|
|---|---|---|---|
| `id` | `SERIAL PRIMARY KEY` | Уникальный идентификатор | — | `session_id` | `INTEGER` | Внешний ключ к сессии | — | `timestamp` | `TIMESTAMP NOT NULL` | Временна́я метка снимка | — | `packets_per_s` | `REAL` | Пакетов в секунду | — | `total_packets` | `BIGINT` | Всего пакетов (нарастающий итог) | — | `current_label` | `INTEGER` | Текущая метка классификации |"""

t211_new = """| Поле | Тип PostgreSQL | Описание |
|---|---|---|
| `id` | `SERIAL PRIMARY KEY` | Уникальный идентификатор |
| `session_id` | `INTEGER` | Внешний ключ к сессии |
| `timestamp` | `TIMESTAMP NOT NULL` | Временна́я метка снимка |
| `packets_per_s` | `REAL` | Пакетов в секунду |
| `total_packets` | `BIGINT` | Всего пакетов (нарастающий итог) |
| `current_label` | `INTEGER` | Текущая метка классификации |"""

text = text.replace(t28_old, t28_new)
text = text.replace(t29_old, t29_new)
text = text.replace(t210_old, t210_new)
text = text.replace(t211_old, t211_new)

with open('docs/academic/ВКР_черновик_claude.md', 'w', encoding='utf-8') as f:
    f.write(text)

print("Tables fixed.")
