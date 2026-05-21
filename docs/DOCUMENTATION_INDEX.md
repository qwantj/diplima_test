# Индекс документации

**Версия:** 2.2  
**Дата актуализации:** 21.05.2026

---

## Документация по DDoS Detection System

### 📖 Основные документы

| Документ | Описание | Аудитория |
|---|---|---|
| [README.md](../README.md) | Обзор проекта, быстрый старт, структура | Все |
| [DOCUMENTATION.md](DOCUMENTATION.md) | Полная техническая спецификация | Все |
| [USER_GUIDE.md](USER_GUIDE.md) | Руководство по использованию системы | Пользователи |

### 🏗️ Архитектура и дизайн

| Документ | Описание | Аудитория |
|---|---|---|
| [ARCHITECTURE.md](ARCHITECTURE.md) | Детальная архитектура, многопоточность, зависимости | Разработчики |
| [API_REFERENCE.md](API_REFERENCE.md) | Протокол обмена Collector↔Monitor (TCP/JSON) | Разработчики |
| [CLASS_REFERENCE.md](CLASS_REFERENCE.md) | Справочник по всем классам и методам | Разработчики |
| [DATABASE_SCHEMA.md](DATABASE_SCHEMA.md) | Схема PostgreSQL, SQL-запросы, управление данными | Разработчики, Администраторы |

### 🔬 Алгоритмы и ML

| Документ | Описание | Аудитория |
|---|---|---|
| [FeatureExtractionAlgorithm.md](FeatureExtractionAlgorithm.md) | Алгоритм извлечения 8 признаков, нормализация | Разработчики, Исследователи |

### 🔧 Разработка

| Документ | Описание | Аудитория |
|---|---|---|
| [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) | Настройка среды, соглашения, добавление признаков и виджетов | Разработчики |
| [TESTING.md](TESTING.md) | Тестирование системы | Разработчики |

### 🚀 Установка и эксплуатация

| Документ | Описание | Аудитория |
|---|---|---|
| [windows-setup.md](windows-setup.md) | Пошаговая установка на Windows | Администраторы |
| [TROUBLESHOOTING.md](TROUBLESHOOTING.md) | Устранение типичных неполадок | Все |

### 📊 Материалы ВКР

| Документ | Описание |
|---|---|
| [diagrams.md](diagrams.md) | Диаграммы и схемы для документации ВКР |
| [academic/](academic/) | Академические материалы |
| [PLANS/](../PLANS/) | История планирования и реализации |

---

## Карта зависимостей документов

```
README.md
│
├── DOCUMENTATION.md (основная спецификация)
│   ├── ARCHITECTURE.md
│   │   ├── API_REFERENCE.md
│   │   └── CLASS_REFERENCE.md
│   ├── DATABASE_SCHEMA.md
│   └── FeatureExtractionAlgorithm.md
│
├── USER_GUIDE.md
├── DEVELOPER_GUIDE.md
│   ├── CLASS_REFERENCE.md
│   └── FeatureExtractionAlgorithm.md
│
├── windows-setup.md
└── TROUBLESHOOTING.md
```

---

## Статус документов

| Документ | Статус | Последнее обновление |
|---|---|---|
| README.md | ✅ Актуален | 21.05.2026 |
| DOCUMENTATION.md | ✅ Актуален | 21.05.2026 |
| ARCHITECTURE.md | ✅ Новый | 21.05.2026 |
| API_REFERENCE.md | ✅ Актуален | 21.05.2026 |
| CLASS_REFERENCE.md | ✅ Новый | 21.05.2026 |
| DATABASE_SCHEMA.md | ✅ Новый | 21.05.2026 |
| FeatureExtractionAlgorithm.md | ✅ Актуален | 21.05.2026 |
| DEVELOPER_GUIDE.md | ✅ Актуален | 21.05.2026 |
| USER_GUIDE.md | ✅ Актуален | 21.05.2026 |
| TROUBLESHOOTING.md | ✅ Актуален | 21.05.2026 |
| windows-setup.md | ⚠️ Требует проверки | 08.04.2026 |
| TESTING.md | ⚠️ Требует обновления | 08.04.2026 |
| diagrams.md | ℹ️ ВКР-материалы | 08.04.2026 |